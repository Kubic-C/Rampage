#pragma once

#include "../../common/common.hpp"

RAMPAGE_START

struct PlayerComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component);
  static void deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component);
  static void fromJson(Ref component, AssetLoader loader, const json& compJson);

  Vec2 mouse = {0, 0};
  float accel = 1.0f;
  float maxSpeed = 5.0f;
};

RAMPAGE_END
