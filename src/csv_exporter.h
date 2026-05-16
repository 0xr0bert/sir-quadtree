#pragma once

#include "agent.h"
#include <fstream>
#include <string>
#include <vector>

class CSVExporter {
public:
  CSVExporter(const std::string& macro_path, const std::string& micro_path)
      : macro_file_(macro_path), micro_file_(micro_path) {
    if (macro_file_.is_open()) {
      macro_file_ << "Time,Susceptible,Infected,Recovered\n";
    }
    if (micro_file_.is_open()) {
      micro_file_ << "Time,AgentID,X,Y,State\n";
    }
  }

  void write_macro(double time, int s, int i, int r) {
    if (macro_file_.is_open()) {
      macro_file_ << time << "," << s << "," << i << "," << r << "\n";
    }
  }

  void write_micro(double time, const std::vector<Agent>& agents) {
    if (micro_file_.is_open()) {
      for (const auto& a : agents) {
        micro_file_ << time << "," << a.id << "," << a.position.x << ","
                    << a.position.y << "," << static_cast<int>(a.state) << "\n";
      }
    }
  }

private:
  std::ofstream macro_file_;
  std::ofstream micro_file_;
};
