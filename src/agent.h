#pragma once

#include "sir_state.h"
#include "vec2.h"

struct Agent {
  Vec2 position;
  Vec2 velocity;
  SirState state;
  double time_in_state;
};
