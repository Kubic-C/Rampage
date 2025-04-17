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

struct Tile {
  b2ShapeId shapeId = b2_nullShapeId;
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

struct TileSpriteComponent {
  u16 texIndex = 0;
  Vec2 offset = Vec2(0);
  float rot = 0;
};

enum BaseLayerIndex {
  BASE_LAYER = 0,
  TURRET_LAYER = 1
};

struct LayeredSpriteComponent {
  static constexpr size_t maxLayered = 3;
  size_t count = 0;
  TileSpriteComponent layers[maxLayered];
};