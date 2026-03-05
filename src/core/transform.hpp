#pragma once

#include <cmath>
#include "../common/ecs/ecs.hpp"
#include "../common/math.hpp"
#include "../common/schema/rampage.capnp.h"

RAMPAGE_START

struct TransformComponent : public Transform {
  static void serialize(capnp::MessageBuilder& builder, Ref component);
  static void deserialize(capnp::MessageReader& reader, const IdMapper& idMapper, Ref component);
  static void fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& compJson);
  TransformComponent& operator=(const Transform& transform);
};

RAMPAGE_END
