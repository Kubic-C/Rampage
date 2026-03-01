#pragma once

#include "../../common/common.hpp"

RAMPAGE_START

struct CircleRenderComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component);
  static void deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component);
  static void fromJson(Ref component, AssetLoader loader, const json& compJson);

  float radius = 0.0f;
  Vec2 offset = Vec2(0);
  float z = 1;
  glm::vec3 color = glm::vec3(1.0f, 0.0f, 0.0f);
};

struct RectangleRenderComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component);
  static void deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component);
  static void fromJson(Ref component, AssetLoader loader, const json& compJson);

  float hw = 0.0f;
  float hh = 0.0f;
  float z = 1;
  glm::vec3 color = glm::vec3(1.0f, 0.0f, 0.0f);
};

RAMPAGE_END
