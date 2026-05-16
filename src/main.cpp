#include "simulation.h"
#include <iostream>
#include <string>
#include <vector>

int main(const int argc, const char* const argv[]) {
  if (argc < 9) {
    std::cerr << "Usage: " << argv[0]
              << " <num_agents> <width> <height> <infection_radius> "
                 "<infection_probability> <recovery_time> <dt> <num_steps>\n";
    return 1;
  }

  try {
    const int num_agents = std::stoi(argv[1]);
    const SimulationConfig config{
        .width = std::stod(argv[2]),
        .height = std::stod(argv[3]),
        .infection_radius = std::stod(argv[4]),
        .infection_probability = std::stod(argv[5]),
        .recovery_time = std::stod(argv[6]),
        .dt = std::stod(argv[7]),
    };
    const int num_steps = std::stoi(argv[8]);

    Simulation sim(num_agents, config);

    for (int i = 0; i < num_steps; ++i) {
      sim.step();

      // Optional: Print progress every 100 steps
      if (i % 100 == 0) {
        int infected = 0;
        for (const auto& agent : sim.get_agents()) {
          if (agent.state == SirState::INFECTED)
            infected++;
        }
        std::cout << "Step " << i << ": " << infected << " agents infected."
                  << std::endl;
      }
    }

    std::cout << "Simulation completed after " << num_steps << " steps."
              << std::endl;

  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
