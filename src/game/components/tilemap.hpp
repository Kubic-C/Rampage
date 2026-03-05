#pragma once

#include "../../core/module.hpp"
#include "body.hpp"

RAMPAGE_START

constexpr glm::vec2 tileSize = glm::vec2(0.5f, 0.5f);

constexpr std::array<glm::ivec3, 4> tileDirectionPos = {
  glm::ivec3(0, 1, 0), 
  glm::ivec3(1, 0, 0), 
  glm::ivec3(0, -1, 0), 
  glm::ivec3(-1, 0, 0)
};

enum TileDirection: u8 {
  Up = 0,
  Right = 1,
  Down = 2,
  Left = 3,
};

inline TileDirection getOppositeDirection(TileDirection dir) {
  switch(dir) {
    case TileDirection::Up: return TileDirection::Down;
    case TileDirection::Right: return TileDirection::Left;
    case TileDirection::Down: return TileDirection::Up;
    case TileDirection::Left: return TileDirection::Right;
  }
}

struct MultiTileComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component);
  static void deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component);
  static void fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& compJson);

  glm::vec2 calculateCentroid() const {
    glm::vec2 sum(0.0f);
    for (const auto& pos : occupiedPositions) {
      sum += glm::vec2(pos);
    }
    return sum / static_cast<float>(occupiedPositions.size());
  }

  std::vector<glm::ivec3> occupiedPositions;  // All grid cells this entity occupies
  glm::ivec3 anchorPos;  // Primary position
};

struct TileComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component);
  static void deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component);
  static void fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& compJson);

  bool collidable = true;
  b2ShapeId shapeId;
  glm::ivec3 pos = {0, 0, 0};
  EntityId parent = 0;
  b2SurfaceMaterial material = b2DefaultSurfaceMaterial();
};

struct TilemapComponent {
  static Vec2 getLocalTileCenter(const glm::ivec2& tilePos) {
    return static_cast<glm::vec2>(tilePos) * tileSize + tileSize * 0.5f;
  }

  static glm::ivec3 getNearestTile(const glm::vec2& localPos, float z = 0.0f) {
    return glm::ivec3(glm::floor(localPos / tileSize), static_cast<int>(z));
  }

  static void serialize(capnp::MessageBuilder& builder, Ref component);
  static void deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component);
  static void fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& compJson);
  static void copy(Ref src, Ref dst);

  static constexpr int maxTopLayer = 1;
  static constexpr int minTopLayer = -1;
  OpenMap<glm::ivec3, EntityId>  tiles;
};

RAMPAGE_END
