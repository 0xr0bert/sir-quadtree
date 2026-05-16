#include "quadtree.h"

Quadtree::Quadtree(const AABB& boundary, size_t capacity)
    : boundary_(boundary), capacity_(capacity) {}

void Quadtree::subdivide() {
  const double hw = boundary_.half_width / 2.0;
  const double hh = boundary_.half_height / 2.0;
  const double x = boundary_.center.x;
  const double y = boundary_.center.y;

  north_west_ = std::make_unique<Quadtree>(AABB{{x - hw, y + hh}, hw, hh}, capacity_);
  north_east_ = std::make_unique<Quadtree>(AABB{{x + hw, y + hh}, hw, hh}, capacity_);
  south_west_ = std::make_unique<Quadtree>(AABB{{x - hw, y - hh}, hw, hh}, capacity_);
  south_east_ = std::make_unique<Quadtree>(AABB{{x + hw, y - hh}, hw, hh}, capacity_);

  is_divided_ = true;
}

bool Quadtree::insert(size_t agent_index, const std::vector<Agent>& agents) {
  if (!boundary_.contains(agents[agent_index].position)) {
    return false;
  }

  if (agent_indices_.size() < capacity_ && !is_divided_) {
    agent_indices_.push_back(agent_index);
    return true;
  }

  if (!is_divided_) {
    subdivide();
    // Move existing agents to children
    for (size_t idx : agent_indices_) {
      if (north_west_->insert(idx, agents)) continue;
      if (north_east_->insert(idx, agents)) continue;
      if (south_west_->insert(idx, agents)) continue;
      if (south_east_->insert(idx, agents)) continue;
    }
    agent_indices_.clear();
  }

  if (north_west_->insert(agent_index, agents)) return true;
  if (north_east_->insert(agent_index, agents)) return true;
  if (south_west_->insert(agent_index, agents)) return true;
  if (south_east_->insert(agent_index, agents)) return true;

  return false;
}

void Quadtree::query_range(const AABB& range, std::vector<size_t>& found_indices,
                           const std::vector<Agent>& agents) const {
  if (!boundary_.intersects(range)) {
    return;
  }

  for (size_t idx : agent_indices_) {
    if (range.contains(agents[idx].position)) {
      found_indices.push_back(idx);
    }
  }

  if (is_divided_) {
    north_west_->query_range(range, found_indices, agents);
    north_east_->query_range(range, found_indices, agents);
    south_west_->query_range(range, found_indices, agents);
    south_east_->query_range(range, found_indices, agents);
  }
}
