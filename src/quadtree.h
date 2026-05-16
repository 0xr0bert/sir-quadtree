#pragma once

#include "agent.h"
#include "vec2.h"
#include <memory>
#include <vector>

struct AABB {
  Vec2 center;
  double half_width;
  double half_height;

  [[nodiscard]] bool contains(const Vec2& point) const {
    return (point.x >= center.x - half_width && point.x <= center.x + half_width &&
            point.y >= center.y - half_height && point.y <= center.y + half_height);
  }

  [[nodiscard]] bool intersects(const AABB& other) const {
    return !(other.center.x - other.half_width > center.x + half_width ||
             other.center.x + other.half_width < center.x - half_width ||
             other.center.y - other.half_height > center.y + half_height ||
             other.center.y + other.half_height < center.y - half_height);
  }
};

class Quadtree {
public:
  Quadtree(const AABB& boundary, size_t capacity);

  bool insert(size_t agent_index, const std::vector<Agent>& agents);
  void query_range(const AABB& range, std::vector<size_t>& found_indices,
                   const std::vector<Agent>& agents) const;

private:
  void subdivide();

  AABB boundary_;
  size_t capacity_;
  std::vector<size_t> agent_indices_;
  bool is_divided_ = false;

  std::unique_ptr<Quadtree> north_west_;
  std::unique_ptr<Quadtree> north_east_;
  std::unique_ptr<Quadtree> south_west_;
  std::unique_ptr<Quadtree> south_east_;
};
