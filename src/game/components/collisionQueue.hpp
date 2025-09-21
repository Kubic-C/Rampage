#pragma once

#include "../../common/common.hpp"

RAMPAGE_START

struct CollisionQueueComponent {
  struct Collision {
    EntityId primary = 0; // The entity which "made" the submission
    EntityId secondary = 0; // The other entity
  };

  std::vector<Collision> queue;
};

struct SubmitToCollisionQueueComponent {
  EntityId queue;
};

RAMPAGE_END