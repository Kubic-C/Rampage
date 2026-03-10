#pragma once

#include "../../common/common.hpp"

RAMPAGE_START

struct ArrowComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component);
  static void deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component);
  static void fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& jsonValue);

  Vec2 dir = Vec2(1.0f, 0.0f);
  float cost = std::numeric_limits<float>::max();
  u32 gen = 0;
  u32 tileCost = 0;
};

struct VectorTilemapPathfinding {
  struct Node {
    Vec2 dir = Vec2(1.0f, 0.0f);
    float cost = std::numeric_limits<float>::max();
    u32 gen = 0;
  };

  glm::ivec2 oldTarget = {0, 0};
  u32 curGen = 0;
  OpenMap<glm::ivec2, Node> nodes; // tile pos -> node

  static void serialize(capnp::MessageBuilder& builder, Ref component);
  static void deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component);
  static void fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& jsonValue);
};

struct PrimaryTargetTag : SerializableTag, JsonableTag {
  PrimaryTargetTag() = default;
};

struct SeekPrimaryTargetTag : SerializableTag, JsonableTag {
  SeekPrimaryTargetTag() = default;
};

RAMPAGE_END

