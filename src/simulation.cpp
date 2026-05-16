#include "simulation.h"
#include "quadtree.h"
#include <algorithm>
#include <cmath>

Simulation::Simulation(const int num_agents, const SimulationConfig& config)
    : config_(config), gen_(std::random_device{}()) {
  std::uniform_real_distribution<double> dist_x(0.0, config_.width);
  std::uniform_real_distribution<double> dist_y(0.0, config_.height);
  std::uniform_real_distribution<double> dist_v(-1.0, 1.0);

  agents_.reserve(num_agents);
  for (int i = 0; i < num_agents; ++i) {
    Agent agent;
    agent.position = {dist_x(gen_), dist_y(gen_)};
    agent.velocity = {dist_v(gen_), dist_v(gen_)};
    if (i == 0) {
      agent.state = SirState::INFECTED;
      agent.time_in_state = config_.recovery_time;
    } else {
      agent.state = SirState::SUSCEPTIBLE;
      agent.time_in_state = 0.0;
    }
    agents_.push_back(agent);
  }
  // Initialize write buffer
  agents_next_ = agents_;
}

void Simulation::step() {
  // Step 0: Sync write buffer with current state to ensure all fields are
  // correct
  // (Alternatively, each function could handle all field updates)
  agents_next_ = agents_;

  // Build Quadtree on the read-only agents_ buffer
  const AABB world_boundary{.center = {config_.width / 2.0, config_.height / 2.0},
                             .half_width = config_.width / 2.0,
                             .half_height = config_.height / 2.0};
  Quadtree tree(world_boundary, 8);
  for (size_t i = 0; i < agents_.size(); ++i) {
    tree.insert(i, agents_);
  }

  move();
  process_infections(tree);
  process_recoveries();

  // Swap buffers
  std::swap(agents_, agents_next_);
}

void Simulation::move() {
  for (size_t i = 0; i < agents_.size(); ++i) {
    const auto& agent_read = agents_[i];
    auto& agent_write = agents_next_[i];

    agent_write.position.x = agent_read.position.x + agent_read.velocity.x * config_.dt;
    agent_write.position.y = agent_read.position.y + agent_read.velocity.y * config_.dt;

    // Toroidal wrap around
    if (agent_write.position.x < 0)
      agent_write.position.x += config_.width;
    else if (agent_write.position.x >= config_.width)
      agent_write.position.x -= config_.width;

    if (agent_write.position.y < 0)
      agent_write.position.y += config_.height;
    else if (agent_write.position.y >= config_.height)
      agent_write.position.y -= config_.height;
  }
}

void Simulation::process_infections(const Quadtree& tree) {
  std::uniform_real_distribution<double> prob_dist(0.0, 1.0);
  const double radius_sq = config_.infection_radius * config_.infection_radius;

  for (size_t i = 0; i < agents_.size(); ++i) {
    // Only infected agents from the read buffer can infect others
    if (agents_[i].state == SirState::INFECTED) {
      const AABB query_range{.center = agents_[i].position,
                             .half_width = config_.infection_radius,
                             .half_height = config_.infection_radius};

      std::vector<size_t> nearby_indices;
      tree.query_range(query_range, nearby_indices, agents_);

      for (const size_t j : nearby_indices) {
        // Check read buffer state, write to write buffer
        if (agents_[j].state == SirState::SUSCEPTIBLE) {
          if (agents_[i].position.dist_sq(agents_[j].position) < radius_sq) {
            if (prob_dist(gen_) < config_.infection_probability) {
              agents_next_[j].state = SirState::NEWLY_INFECTED;
              agents_next_[j].time_in_state = config_.recovery_time;
            }
          }
        }
      }
    }
  }

  // Resolve NEWLY_INFECTED in the write buffer
  for (auto& agent : agents_next_) {
    if (agent.state == SirState::NEWLY_INFECTED) {
      agent.state = SirState::INFECTED;
    }
  }
}

void Simulation::process_recoveries() {
  for (size_t i = 0; i < agents_.size(); ++i) {
    const auto& agent_read = agents_[i];
    auto& agent_write = agents_next_[i];

    if (agent_read.state == SirState::INFECTED) {
      agent_write.time_in_state = agent_read.time_in_state - config_.dt;
      if (agent_write.time_in_state <= 0.0) {
        agent_write.state = SirState::RECOVERED;
        agent_write.time_in_state = 0.0;
      }
    }
  }
}
