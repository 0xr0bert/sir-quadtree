#pragma once

#include "agent.h"
#include <barrier>
#include <thread>
#include <vector>
#include <atomic>

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
  ~Simulation();

  void step();

  [[nodiscard]] const std::vector<Agent>& get_agents() const { return agents_; }

private:
  void worker_thread(unsigned int thread_id);
  void update_range(size_t start, size_t end, const Quadtree& tree);

  std::vector<Agent> agents_;
  std::vector<Agent> agents_next_;
  SimulationConfig config_;
  
  unsigned int num_threads_;
  std::vector<std::thread> workers_;
  
  // Synchronization
  std::barrier<> sync_point_;
  std::atomic<bool> running_{true};
  std::unique_ptr<Quadtree> current_tree_;
};
