#pragma once

#include "../../common/common.hpp"

RAMPAGE_START

struct OnCollisionBeginEvent {};
struct OnCollisionEndEvent {};
struct OnContactEvent {};

struct LastCollisionData {
  EntityId other;
};

RAMPAGE_END
