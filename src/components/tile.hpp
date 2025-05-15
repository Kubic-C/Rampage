#pragma once

#include "../ecs/ecs.hpp"
#include "../utility/math.hpp"
#include "sprite.hpp"

constexpr glm::vec2 tileSize = glm::vec2(baseSpriteScale);

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
  bool enableCollision = false;
  EntityId entity = 0;

  glm::u16vec2 size = glm::u16vec2(1, 1);

  u16 width() const {
    return size.x;
  }

  u16 height() const {
    return size.y;
  }
};

struct Tile {
  b2ShapeId shapeDef = b2_nullShapeId;
  u8 flags = 0;
  EntityId entity = 0;

  union _X {
    u16 w; // the width of the multi tile
    i16 x = 0; // the x position of the main mutile tile
  } x;

  union _Y {
    u16 h; // the height of the multi tile
    i16 y = 0; // the y position of the main multi tile
  } y;

  glm::vec2 size() const {
    return { x.w, y.h };
  }

  glm::vec2 pos() const {
    return { x.x, y.y };
  }

  u16 width() const {
    return x.w;
  }

  u16 height() const {
    return y.h;
  }
};

template<>
struct glz::meta<TileDef> {
  using T = TileDef;
  static constexpr auto value = object(
    "shapeDef", &T::shapeDef,
    "enableCollision", &T::enableCollision,
    "size", &T::size
  );
};
