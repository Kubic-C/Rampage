#pragma once

#include "../../common/common.hpp"

RAMPAGE_START

struct OnCollisionBeginEvent {};
struct OnCollisionEndEvent {};
struct OnContactEvent {};

struct LastCollisionData : SerializableTag, JsonableTag {
  EntityId other = 0;
};

RAMPAGE_END
