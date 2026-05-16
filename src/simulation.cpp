#include "simulation.h"
#include "quadtree.h"
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
}

void Simulation::step() {
  move();

  // Build Phase: Create a new root Quadtree spanning the world boundaries.
  const AABB world_boundary{.center = {config_.width / 2.0, config_.height / 2.0},
                             .half_width = config_.width / 2.0,
                             .half_height = config_.height / 2.0};
  Quadtree tree(world_boundary, 8); // Capacity 8

  for (size_t i = 0; i < agents_.size(); ++i) {
    tree.insert(i, agents_);
  }

  process_infections(tree);
  process_recoveries();
}

void Simulation::move() {
  for (auto& agent : agents_) {
    agent.position.x += agent.velocity.x * config_.dt;
    agent.position.y += agent.velocity.y * config_.dt;

    // Toroidal wrap around
    if (agent.position.x < 0)
      agent.position.x += config_.width;
    else if (agent.position.x >= config_.width)
      agent.position.x -= config_.width;

    if (agent.position.y < 0)
      agent.position.y += config_.height;
    else if (agent.position.y >= config_.height)
      agent.position.y -= config_.height;
  }
}

void Simulation::process_infections(const Quadtree& tree) {
  std::uniform_real_distribution<double> prob_dist(0.0, 1.0);
  const double radius_sq = config_.infection_radius * config_.infection_radius;

  for (size_t i = 0; i < agents_.size(); ++i) {
    if (agents_[i].state == SirState::INFECTED) {
      // Create a query AABB centered on agent i's position
      const AABB query_range{.center = agents_[i].position,
                             .half_width = config_.infection_radius,
                             .half_height = config_.infection_radius};

      std::vector<size_t> nearby_indices;
      tree.query_range(query_range, nearby_indices, agents_);

      for (const size_t j : nearby_indices) {
        if (agents_[j].state == SirState::SUSCEPTIBLE) {
          if (agents_[i].position.dist_sq(agents_[j].position) < radius_sq) {
            if (prob_dist(gen_) < config_.infection_probability) {
              agents_[j].state = SirState::NEWLY_INFECTED;
              agents_[j].time_in_state = config_.recovery_time;
            }
          }
        }
      }
    }
  }

  // Resolve NEWLY_INFECTED to INFECTED
  for (auto& agent : agents_) {
    if (agent.state == SirState::NEWLY_INFECTED) {
      agent.state = SirState::INFECTED;
    }
  }
}

void Simulation::process_recoveries() {
  for (auto& agent : agents_) {
    if (agent.state == SirState::INFECTED) {
      agent.time_in_state -= config_.dt;
      if (agent.time_in_state <= 0.0) {
        agent.state = SirState::RECOVERED;
        agent.time_in_state = 0.0;
      }
    }
  }
}
