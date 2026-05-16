#include "simulation.h"
#include "quadtree.h"

Simulation::Simulation(const int num_agents, const SimulationConfig& config)
    : config_(config),
      num_threads_(4),
      sync_point_(5) { // 4 workers + 1 main

  const double hw = config_.width / 2.0;
  const double hh = config_.height / 2.0;

  // 0: NW, 1: NE, 2: SW, 3: SE
  regions_.push_back(std::make_unique<RegionSubsystem>(
      AABB{{hw / 2.0, 3.0 * hh / 2.0}, hw / 2.0, hh / 2.0}));
  regions_.push_back(std::make_unique<RegionSubsystem>(
      AABB{{3.0 * hw / 2.0, 3.0 * hh / 2.0}, hw / 2.0, hh / 2.0}));
  regions_.push_back(std::make_unique<RegionSubsystem>(
      AABB{{hw / 2.0, hh / 2.0}, hw / 2.0, hh / 2.0}));
  regions_.push_back(std::make_unique<RegionSubsystem>(
      AABB{{3.0 * hw / 2.0, hh / 2.0}, hw / 2.0, hh / 2.0}));

  std::mt19937 gen(std::random_device{}());
  std::uniform_real_distribution<double> dist_x(0.0, config_.width);
  std::uniform_real_distribution<double> dist_y(0.0, config_.height);
  std::uniform_real_distribution<double> dist_v(-1.0, 1.0);

  for (int i = 0; i < num_agents; ++i) {
    Agent agent{};
    agent.id = static_cast<uint64_t>(i);
    agent.position = {dist_x(gen), dist_y(gen)};
    agent.velocity = {dist_v(gen), dist_v(gen)};
    if (i == 0) {
      agent.state = SirState::INFECTED;
      agent.time_in_state = config_.recovery_time;
    } else {
      agent.state = SirState::SUSCEPTIBLE;
      agent.time_in_state = 0.0;
    }

    const int r_idx = get_region_index(agent.position);
    regions_[r_idx]->agents.push_back(agent);
  }

  // Spawn workers
  for (unsigned int i = 0; i < num_threads_; ++i) {
    workers_.emplace_back(&Simulation::worker_thread, this, i);
  }
}

Simulation::~Simulation() {
  running_ = false;
  sync_point_.arrive_and_drop();
  for (auto& w : workers_) {
    if (w.joinable()) {
      w.join();
    }
  }
}

int Simulation::get_region_index(const Vec2& pos) const {
  const bool right = pos.x >= config_.width / 2.0;
  const bool top = pos.y >= config_.height / 2.0;
  if (top) {
    return right ? 1 : 0; // NE : NW
  } else {
    return right ? 3 : 2; // SE : SW
  }
}

void Simulation::step() {
  // Phase 1: Move & Departure
  sync_point_.arrive_and_wait();
  if (!running_)
    return;

  // Phase 2: Assimilation & Tree Build
  sync_point_.arrive_and_wait();

  // Phase 3: Infection & State Update
  sync_point_.arrive_and_wait();

  // Phase 4: Buffer Swap
  sync_point_.arrive_and_wait();

  // Phase 5: Ready for next step
  sync_point_.arrive_and_wait();
}

void Simulation::worker_thread(unsigned int region_id) {
  auto& local = *regions_[region_id];

  while (running_) {
    // Phase 1: Move & Departure
    sync_point_.arrive_and_wait();
    if (!running_)
      break;

    local.agents_next.clear();
    for (const auto& a : local.agents) {
      Agent moved = a;
      moved.position.x += moved.velocity.x * config_.dt;
      moved.position.y += moved.velocity.y * config_.dt;

      // Toroidal wrap
      if (moved.position.x < 0)
        moved.position.x += config_.width;
      else if (moved.position.x >= config_.width)
        moved.position.x -= config_.width;

      if (moved.position.y < 0)
        moved.position.y += config_.height;
      else if (moved.position.y >= config_.height)
        moved.position.y -= config_.height;

      const int target_idx = get_region_index(moved.position);
      if (target_idx == static_cast<int>(region_id)) {
        local.agents_next.push_back(moved);
      } else {
        regions_[target_idx]->add_to_inbox(moved);
      }
    }

    // Phase 2: Assimilation & Tree Build
    sync_point_.arrive_and_wait();

    {
      std::lock_guard<std::mutex> lock(local.inbox_mutex);
      for (const auto& incoming : local.inbox) {
        local.agents_next.push_back(incoming);
      }
      local.inbox.clear();
    }

    local.tree = std::make_unique<Quadtree>(local.boundary, 8);
    for (size_t i = 0; i < local.agents_next.size(); ++i) {
      local.tree->insert(i, local.agents_next);
    }

    // Phase 3: Infection & State Update
    sync_point_.arrive_and_wait();
    process_regional_infections(region_id);

    // Phase 4: Buffer Swap
    sync_point_.arrive_and_wait();
    std::swap(local.agents, local.agents_next);

    // Phase 5: Ready
    sync_point_.arrive_and_wait();
  }
}

void Simulation::process_regional_infections(unsigned int region_id) {
  auto& local = *regions_[region_id];
  std::mt19937 local_gen(std::random_device{}());
  std::uniform_real_distribution<double> prob_dist(0.0, 1.0);
  const double radius_sq = config_.infection_radius * config_.infection_radius;

  for (auto& agent : local.agents_next) {
    if (agent.state == SirState::SUSCEPTIBLE) {
      const AABB query_range{.center = agent.position,
                             .half_width = config_.infection_radius,
                             .half_height = config_.infection_radius};

      bool infected = false;

      // Helper to check a specific region
      auto check_region = [&](const RegionSubsystem& region) {
        std::vector<size_t> indices;
        region.tree->query_range(query_range, indices, region.agents_next);
        for (const size_t idx : indices) {
          if (region.agents_next[idx].state == SirState::INFECTED) {
            if (agent.position.dist_sq(region.agents_next[idx].position) <
                radius_sq) {
              if (prob_dist(local_gen) < config_.infection_probability) {
                return true;
              }
            }
          }
        }
        return false;
      };

      // Check local region
      if (check_region(local)) {
        agent.state = SirState::NEWLY_INFECTED;
        agent.time_in_state = config_.recovery_time;
        infected = true;
      }

      // Check neighbor regions if query overlaps
      if (!infected) {
        for (unsigned int n_idx = 0; n_idx < 4; ++n_idx) {
          if (n_idx == region_id)
            continue;
          if (query_range.intersects(regions_[n_idx]->boundary)) {
            if (check_region(*regions_[n_idx])) {
              agent.state = SirState::NEWLY_INFECTED;
              agent.time_in_state = config_.recovery_time;
              infected = true;
              break;
            }
          }
        }
      }
    } else if (agent.state == SirState::INFECTED) {
      agent.time_in_state -= config_.dt;
      if (agent.time_in_state <= 0.0) {
        agent.state = SirState::RECOVERED;
        agent.time_in_state = 0.0;
      }
    }
  }

  // Resolve NEWLY_INFECTED
  for (auto& agent : local.agents_next) {
    if (agent.state == SirState::NEWLY_INFECTED) {
      agent.state = SirState::INFECTED;
    }
  }
}

std::vector<Agent> Simulation::get_agents() const {
  std::vector<Agent> all;
  for (const auto& r : regions_) {
    all.insert(all.end(), r->agents.begin(), r->agents.end());
  }
  return all;
}
