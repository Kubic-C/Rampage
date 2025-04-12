#pragma once

#include "utility/ecs.hpp"
#include "utility/math.hpp"

struct PosComponent : Vec2 {
  PosComponent() { x = y = 0;  }

  PosComponent(float scalar)
    : Vec2(scalar) {}

  PosComponent(float x, float y)
    : Vec2(x, y) {}

  PosComponent(const glm::vec2& other)
    : Vec2(other) {}

  PosComponent(const b2Vec2& other)
    : Vec2(other.x, other.y) {}

  PosComponent& operator=(const b2Vec2& other) {
    x = other.x;
    y = other.y;
    return *this;
  }
};

struct RotComponent : Rot {
  RotComponent() { s = 0; c = 1; }

  RotComponent(const Rot& rot)
    : Rot(rot) {}

  RotComponent& operator=(const b2Rot& other) {
    s = other.s;
    c = other.c;
    return *this;
  }
};
