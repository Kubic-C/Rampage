#pragma once

#include "../../common/common.hpp"

RAMPAGE_START

struct PlayerComponent {
  Vec2 mouse = {0, 0};
  float accel = 1.0f;
  float maxSpeed = 5.0f;
};

RAMPAGE_END