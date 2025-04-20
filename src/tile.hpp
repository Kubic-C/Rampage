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

static constexpr glm::u8vec2 maxDim = { 15, 15 };

struct TileDef {
  b2ShapeDef shapeDef = b2DefaultShapeDef();
  u8 flags = 0;
  EntityId entity;

  union {
    u8 width; // the width of the multi tile
    i16 posx; // the x position of the main mutile tile
  };

  union {
    u8 height; // the height of the multi tile
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
