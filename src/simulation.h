#pragma once

#include "agent.h"
#include "region_subsystem.h"
#include <atomic>
#include <barrier>
#include <memory>
#include <random>
#include <thread>
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
  Simulation(int num_agents, const SimulationConfig& config);
  ~Simulation();

  void step();

  [[nodiscard]] std::vector<Agent> get_agents() const;

private:
  void worker_thread(unsigned int region_id);
  void process_regional_infections(unsigned int region_id);
  [[nodiscard]] int get_region_index(const Vec2& pos) const;

  std::vector<std::unique_ptr<RegionSubsystem>> regions_;
  SimulationConfig config_;

  unsigned int num_threads_;
  std::vector<std::thread> workers_;

  // Synchronization
  std::barrier<> sync_point_;
  std::atomic<bool> running_{true};
};
