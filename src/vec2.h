#pragma once

struct Vec2 {
  double x, y;

  Vec2 operator+(const Vec2& other) const {
    return {x + other.x, y + other.y};
  }

  Vec2 operator*(const double scalar) const {
    return {x * scalar, y * scalar};
  }

  [[nodiscard]] double dist_sq(const Vec2& other) const {
    const double dx = x - other.x;
    const double dy = y - other.y;
    return dx * dx + dy * dy;
  }
};

inline Vec2 operator*(const double scalar, const Vec2& vec) {
  return vec * scalar;
}
