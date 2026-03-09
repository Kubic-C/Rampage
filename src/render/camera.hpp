#pragma once

#include "../common/common.hpp"

RAMPAGE_START

struct CameraInUseTag : SerializableTag, JsonableTag {};

struct ViewRect {
  Vec2 center = Vec2(0);
  Vec2 halfDim = Vec2(std::numeric_limits<float>::max());

  // Check if a point is inside this rectangle
  bool contains(const Vec2& point) const {
    return std::abs(point.x - center.x) <= halfDim.x && std::abs(point.y - center.y) <= halfDim.y;
  }

  // Check if another rectangle (centered at 'point', with half-size 'halfSize') is fully inside this
  // rectangle
  bool intersects(const Vec2& point, const Vec2& halfSize) const {
    return std::abs(point.x - center.x) <= (halfDim.x + halfSize.x) &&
        std::abs(point.y - center.y) <= (halfDim.y + halfSize.y);
  }
};

struct CameraComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component);
  static void deserialize(capnp::MessageReader& reader, const IdMapper& idMapper, Ref component);
  static void fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& compJson);
  CameraComponent();
  glm::mat4 view(const Transform& transform, const Vec2& screenDim, float z = 1.0f) const;
  ViewRect getViewRect(const Transform& transform, const Vec2& screenDim, float z = 1.0f) const;
  void safeZoom(float amount);

  float m_zoom;
  float m_rot;
};

RAMPAGE_END
