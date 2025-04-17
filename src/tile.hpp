#pragma once

#include "transform.hpp"

constexpr float tileSidelength = 0.5f;
constexpr glm::vec2 tileSize = glm::vec2(tileSidelength);

enum TileFlags : u8 {
  IS_MULTI_TILE = 1 << 0,
  IS_MAIN_TILE = 1 << 1,
  IS_COLLIDABLE = 1 << 2,
  MAX_BITS = 3
};

inline TileFlags getTileFlagFromString(const std::string& str) {
  if (str == "IS_MULTI_TILE")
    return TileFlags::IS_MULTI_TILE;
  if (str == "IS_COLLIDABLE")
    return TileFlags::IS_COLLIDABLE;
  return (TileFlags)std::numeric_limits<u8>::max();
}

struct TileDef {
  b2ShapeDef shapeDef = b2DefaultShapeDef();
  u8 flags = 0;
  EntityId entity = 0;

  union {
    u16 width; // the width of the multi tile
    i16 posx; // the x position of the main mutile tile
  };

  union {
    u16 height; // the height of the multi tile
    i16 posy; // the y position of the main multi tile
  };

  glm::vec2 size() const {
    return { width, height };
  }
};

struct Tile {
  b2ShapeId shapeDef = b2_nullShapeId;
  u8 flags = 0;
  EntityId entity = 0;

  union {
    u16 width; // the width of the multi tile
    i16 posx; // the x position of the main mutile tile
  };

  union {
    u16 height; // the height of the multi tile
    i16 posy; // the y position of the main multi tile
  };

  glm::vec2 size() const {
    return { width, height };
  }
};

enum BaseLayerIndex {
  BASE_LAYER = 0,
  TURRET_LAYER = 1
};

struct SpriteLayer {
  SpriteLayer() = default;
  SpriteLayer(u32 texIndex, glm::vec2 offset, float rot)
    : texIndex(texIndex), offset(offset), rot(rot) {}

  u32 texIndex = 0;
  glm::vec2 offset = { 0.0f, 0.0f };
  float rot = 0.0f;
};

struct SpriteComponent {
  static constexpr size_t MAX_SPRITE_LAYERS = 4;
  SpriteLayer layers[MAX_SPRITE_LAYERS];
  float zOffset = -1.0f;
  u8 layerCount = 0;

  void addLayer(const SpriteLayer& layer) {
    assert(layerCount < MAX_SPRITE_LAYERS && "Too many sprite layers!");
    layers[layerCount++] = layer;
  }

  void addLayer(u32 texIndex, glm::vec2 offset = Vec2(0), float rot = 0) {
    assert(layerCount < MAX_SPRITE_LAYERS && "Too many sprite layers!");
    layers[layerCount++] = SpriteLayer(texIndex, offset, rot);
  }

  SpriteLayer& getLast() {
    return layers[layerCount - 1];
  }

  const SpriteLayer& operator[](size_t index) const {
    assert(index < layerCount);
    return layers[index];
  }
};