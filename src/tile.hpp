#pragma once

#include "utility/base.hpp"
#include "utility/ecs.hpp"

constexpr float tileSidelength = 0.5f;
constexpr glm::vec2 tileSize = glm::vec2(tileSidelength);

enum TileFlags : u8 {
  IS_MULTI_TILE = 1 << 0,
  IS_MAIN_TILE = 1 << 1,
  IS_COLLIDABLE = 1 << 2,
  MAX_BITS = 3
};

struct Tile {
  b2ShapeId shapeId;
  u8 flags;
  EntityId entity;

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

struct SpriteComponent {
  u16 texIndex;
};