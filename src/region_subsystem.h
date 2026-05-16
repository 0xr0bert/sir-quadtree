#pragma once

#include "agent.h"
#include "quadtree.h"
#include <memory>
#include <mutex>
#include <vector>

struct RegionSubsystem {
  AABB boundary;
  std::vector<Agent> agents;
  std::vector<Agent> agents_next;
  std::unique_ptr<Quadtree> tree;

  std::vector<Agent> inbox;
  std::mutex inbox_mutex;

  explicit RegionSubsystem(const AABB& b) : boundary(b) {}

  void add_to_inbox(const Agent& a) {
    std::lock_guard<std::mutex> lock(inbox_mutex);
    inbox.push_back(a);
  }
};
