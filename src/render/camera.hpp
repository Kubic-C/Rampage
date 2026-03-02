#pragma once

#include "../common/common.hpp"

RAMPAGE_START

struct CameraInUseTag : SerializableTag, JsonableTag {};

struct CameraComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component);
  static void deserialize(capnp::MessageReader& reader, Ref component);
  static void fromJson(Ref component, AssetLoader loader, const json& compJson);
  CameraComponent();
  glm::mat4 view(const b2Transform& transform, const Vec2& screenDim, float z = 1.0f) const;
  void safeZoom(float amount);

  float m_zoom;
  float m_rot;
};

RAMPAGE_END