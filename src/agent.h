#pragma once

#include "sir_state.h"
#include "vec2.h"
#include <cstdint>

struct Agent {
  uint64_t id;
  Vec2 position;
  Vec2 velocity;
  SirState state;
  double time_in_state;
};
