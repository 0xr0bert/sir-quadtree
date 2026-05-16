#include "simulation.h"
#include <random>

Simulation::Simulation(const int num_agents, const SimulationConfig &config)
    : config_(config) {
  std::mt19937 gen(std::random_device{}());
  std::uniform_real_distribution<double> dist_x(0.0, config_.width);
  std::uniform_real_distribution<double> dist_y(0.0, config_.height);
  std::uniform_real_distribution<double> dist_v(-1.0, 1.0);

  agents_.reserve(num_agents);
  for (int i = 0; i < num_agents; ++i) {
    Agent agent;
    agent.position = {dist_x(gen), dist_y(gen)};
    agent.velocity = {dist_v(gen), dist_v(gen)};
    agent.state = (i == 0) ? SirState::INFECTED : SirState::SUSCEPTIBLE;
    agent.time_in_state = 0.0;
    agents_.push_back(agent);
  }
}

void Simulation::step() {
  // To be implemented: movement and infection logic
}
