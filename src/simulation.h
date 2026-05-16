#pragma once

#include "agent.h"
#include <random>
#include <vector>

struct SimulationConfig {
  double width;
  double height;
  double infection_radius;
  double infection_probability;
  double recovery_time;
  double dt;
};

class Quadtree; // Forward declaration

class Simulation {
public:
  Simulation(int num_agents, const SimulationConfig& config);

  void step();

  [[nodiscard]] const std::vector<Agent>& get_agents() const { return agents_; }

private:
  void move();
  void process_infections(const Quadtree& tree);
  void process_recoveries();

  std::vector<Agent> agents_;
  SimulationConfig config_;
  std::mt19937 gen_;
};
