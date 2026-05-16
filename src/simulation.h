#pragma once

#include "agent.h"
#include <vector>

struct SimulationConfig {
  double width;
  double height;
  double infection_radius;
  double infection_probability;
  double recovery_time;
  double dt;
};

class Simulation {
public:
  Simulation(int num_agents, const SimulationConfig &config);

  void step();

  [[nodiscard]] const std::vector<Agent>& get_agents() const { return agents_; }

private:
  std::vector<Agent> agents_;
  SimulationConfig config_;
};
