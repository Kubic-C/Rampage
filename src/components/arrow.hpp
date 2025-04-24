#pragma once

#include "../utility/math.hpp"

struct ArrowComponent {
  // Normalized vector
  // points up, down, left, or right
  Vec2 dir = { 1.0f, 0.0f };
  float cost = std::numeric_limits<float>::max();
  u32 generation = 0;
  u32 tileCost = 0;
};

struct PrimaryTargetTag {};
struct SeekPrimaryTargetTag {};