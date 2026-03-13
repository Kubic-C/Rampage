#pragma once

#include "../common/common.hpp"

RAMPAGE_START

struct CameraInUseTag : SerializableTag, JsonableTag {};

struct CameraComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component);
  static void deserialize(capnp::MessageReader& reader, const IdMapper& idMapper, Ref component);
  static void fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& compJson);

  float zoom = 20;
  float rot = 0;
};

RAMPAGE_END
