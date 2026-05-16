#include "simulation.h"
#include "quadtree.h"
#include <random>
#include <algorithm>

Simulation::Simulation(const int num_agents, const SimulationConfig& config)
    : config_(config), 
      num_threads_(std::thread::hardware_concurrency() > 0 ? std::thread::hardware_concurrency() : 1),
      sync_point_(num_threads_ + 1) { // +1 for the main thread

  std::mt19937 gen(std::random_device{}());
  std::uniform_real_distribution<double> dist_x(0.0, config_.width);
  std::uniform_real_distribution<double> dist_y(0.0, config_.height);
  std::uniform_real_distribution<double> dist_v(-1.0, 1.0);

  agents_.reserve(num_agents);
  for (int i = 0; i < num_agents; ++i) {
    Agent agent;
    agent.position = {dist_x(gen), dist_y(gen)};
    agent.velocity = {dist_v(gen), dist_v(gen)};
    if (i == 0) {
      agent.state = SirState::INFECTED;
      agent.time_in_state = config_.recovery_time;
    } else {
      agent.state = SirState::SUSCEPTIBLE;
      agent.time_in_state = 0.0;
    }
    agents_.push_back(agent);
  }
  agents_next_ = agents_;

  // Spawn persistent worker threads
  for (unsigned int i = 0; i < num_threads_; ++i) {
    workers_.emplace_back(&Simulation::worker_thread, this, i);
  }
}

Simulation::~Simulation() {
  running_ = false;
  sync_point_.arrive_and_drop(); // Main thread drops out
  for (auto& w : workers_) {
    w.join();
  }
}

void Simulation::step() {
  agents_next_ = agents_;

  // Build Quadtree (Serial) on the heap to avoid stack escape
  const AABB world_boundary{.center = {config_.width / 2.0, config_.height / 2.0},
                             .half_width = config_.width / 2.0,
                             .half_height = config_.height / 2.0};
  
  current_tree_ = std::make_unique<Quadtree>(world_boundary, 8);
  for (size_t i = 0; i < agents_.size(); ++i) {
    current_tree_->insert(i, agents_);
  }

  // Signal workers to start
  sync_point_.arrive_and_wait();

  // Wait for workers to finish
  sync_point_.arrive_and_wait();

  // Clear tree to ensure no stale pointers are held
  current_tree_.reset();

  std::swap(agents_, agents_next_);
}

void Simulation::worker_thread(unsigned int thread_id) {
  const size_t num_agents = agents_.size();
  const size_t chunk_size = (num_agents + num_threads_ - 1) / num_threads_;
  const size_t start = thread_id * chunk_size;
  const size_t end = std::min(start + chunk_size, num_agents);

  while (running_) {
    // Wait for start signal
    sync_point_.arrive_and_wait();
    
    if (!running_) break;

    if (start < end) {
      update_range(start, end, *current_tree_);
    }

    // Wait for work completion
    sync_point_.arrive_and_wait();
  }
}

void Simulation::update_range(const size_t start, const size_t end,
                              const Quadtree& tree) {
  std::mt19937 local_gen(std::random_device{}());
  std::uniform_real_distribution<double> prob_dist(0.0, 1.0);
  const double radius_sq = config_.infection_radius * config_.infection_radius;

  for (size_t i = start; i < end; ++i) {
    const auto& agent_read = agents_[i];
    auto& agent_write = agents_next_[i];

    agent_write.position.x = agent_read.position.x + agent_read.velocity.x * config_.dt;
    agent_write.position.y = agent_read.position.y + agent_read.velocity.y * config_.dt;

    if (agent_write.position.x < 0) agent_write.position.x += config_.width;
    else if (agent_write.position.x >= config_.width) agent_write.position.x -= config_.width;

    if (agent_write.position.y < 0) agent_write.position.y += config_.height;
    else if (agent_write.position.y >= config_.height) agent_write.position.y -= config_.height;

    if (agent_read.state == SirState::SUSCEPTIBLE) {
      const AABB query_range{.center = agent_read.position,
                             .half_width = config_.infection_radius,
                             .half_height = config_.infection_radius};
      std::vector<size_t> nearby_indices;
      tree.query_range(query_range, nearby_indices, agents_);

      for (const size_t j : nearby_indices) {
        if (agents_[j].state == SirState::INFECTED) {
          if (agent_read.position.dist_sq(agents_[j].position) < radius_sq) {
            if (prob_dist(local_gen) < config_.infection_probability) {
              agent_write.state = SirState::INFECTED;
              agent_write.time_in_state = config_.recovery_time;
              break;
            }
          }
        }
      }
    } else if (agent_read.state == SirState::INFECTED) {
      agent_write.time_in_state = agent_read.time_in_state - config_.dt;
      if (agent_write.time_in_state <= 0.0) {
        agent_write.state = SirState::RECOVERED;
        agent_write.time_in_state = 0.0;
      }
    }
  }
}
