#include "csv_exporter.h"
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
    CSVExporter exporter("macro.csv", "micro.csv");

    double current_time = 0.0;
    for (int step = 0; step < num_steps; ++step) {
      sim.step();

      // Macro view: Count states
      int s = 0, i = 0, r = 0;
      const auto& agents = sim.get_agents();
      for (const auto& a : agents) {
        if (a.state == SirState::SUSCEPTIBLE)
          s++;
        else if (a.state == SirState::INFECTED)
          i++;
        else if (a.state == SirState::RECOVERED)
          r++;
      }
      exporter.write_macro(current_time, s, i, r);
      exporter.write_micro(current_time, agents);

      current_time += config.dt;

      // Optional: Print progress every 100 steps
      if (step % 100 == 0) {
        std::cout << "Step " << step << ": " << i << " agents infected."
                  << std::endl;
      }
    }

    std::cout << "Simulation completed. Data saved to macro.csv and micro.csv"
              << std::endl;

  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
