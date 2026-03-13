#pragma once

#include "../../common/common.hpp"

RAMPAGE_START

struct ShapeVertex {
  glm::vec3 pos;
  glm::vec3 color;
};

struct CircleRenderComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component);
  static void deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component);
  static void fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& compJson);

  float radius = 5.0f;
  Vec2 offset = Vec2(0);
  float z = 1;
  glm::vec3 color = glm::vec3(1.0f, 0.0f, 0.0f);
};

struct RectangleRenderComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component);
  static void deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component);
  static void fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& compJson);

  float hw = 0.5f;
  float hh = 0.5f;
  float z = 1;
  glm::vec3 color = glm::vec3(1.0f, 0.0f, 0.0f);
};

RAMPAGE_END
