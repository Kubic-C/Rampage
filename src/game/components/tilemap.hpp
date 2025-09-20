#pragma once

#include "../../temp/components/transform.hpp"
#include "../components/tile.hpp"
#include "../utility/hashes.hpp"

class TilemapComponent;

struct TileBoundComponent {
  u32 layer;
  glm::i16vec2 pos;
  EntityId parent;
};

class Tilemap {
  friend class TilemapComponent;

  public:
  // Ignores shapeId
  bool insert(EntityWorld& world, b2BodyId body, const glm::i16vec2& pos, EntityId parent,
              const TileDef& clone) {
    u8 flags = 0;
    if (clone.enableCollision)
      flags |= TileFlags::IS_COLLIDABLE;

    return insert(world, body, pos, parent, clone.entity, flags, clone.size, clone.shapeDef);
  }

  bool insert(EntityWorld& world, b2BodyId body, const glm::i16vec2& pos, EntityId parent = 0,
              EntityId entity = 0, u8 tileFlags = TileFlags::IS_COLLIDABLE, const glm::u16vec2& dim = {1, 1},
              const b2ShapeDef& shapeDef = b2DefaultShapeDef()) {
    if (contains(pos))
      return false;

    if (dim.x != 1 || dim.y != 1)
      tileFlags |= TileFlags::IS_MULTI_TILE;

    Tile tile;
    tile.entity = entity;
    tile.flags = tileFlags | TileFlags::IS_MAIN_TILE;
    tile.x.w = dim.x;
    tile.y.h = dim.y;

    if (entity) {
      Entity e = world.get(entity);
      e.add<TileBoundComponent>();
      RefT<TileBoundComponent> tileBound = e.get<TileBoundComponent>();
      tileBound->parent = parent;
      tileBound->pos = pos;
      tileBound->layer = m_layer;
    }

    if (tile.flags & TileFlags::IS_MULTI_TILE) {
      const glm::i16vec2 startPos = pos;
      const glm::i16vec2 endPos = pos + static_cast<glm::i16vec2>(dim);

      Tile subTile;
      subTile.entity = NullEntityId;
      subTile.flags = tileFlags & ~TileFlags::IS_MAIN_TILE;
      subTile.x.x = pos.x;
      subTile.y.y = pos.y;
      subTile.shapeDef = b2_nullShapeId;

      for (i16 y = startPos.y; y < endPos.y; y++) {
        for (i16 x = startPos.x; x < endPos.x; x++) {
          if (contains({x, y}))
            return false;
        }
      }

      for (i16 y = startPos.y; y < endPos.y; y++) {
        for (i16 x = startPos.x; x < endPos.x; x++) {
          if (y == pos.y && x == pos.x)
            continue;

          m_tiles.insert(std::make_pair(glm::i16vec2(x, y), subTile));
        }
      }
    }

    if (tileFlags & TileFlags::IS_COLLIDABLE) {
      glm::vec2 fdim = dim;
      b2Vec2 tilePos = getLocalTileCenter(pos, fdim).b2();
      b2Polygon tilePolygon =
          b2MakeOffsetBox(fdim.x * tileSize.x * 0.5f, fdim.y * tileSize.y * 0.5f, tilePos, Rot(0.0f));
      b2ShapeDef copyShapeDef = shapeDef;
      copyShapeDef.userData = entityToB2Data(entity);
      tile.shapeDef = b2CreatePolygonShape(body, &copyShapeDef, &tilePolygon);
    } else {
      tile.shapeDef = b2_nullShapeId;
    }

    assert(!m_tiles.contains(pos));
    m_tiles.insert(std::make_pair(pos, tile));
    m_mainTiles.insert(pos);

    return true;
  }

  // !ATTENTION! this does not destroy the entity located with the tile
  EntityId erase(const glm::i16vec2& pos) {
    Tile copy = find(pos);
    EntityId entity = 0;

    if (copy.flags & TileFlags::IS_MULTI_TILE) {
      if (!(copy.flags & TileFlags::IS_MAIN_TILE)) {
        return erase(copy.pos());
      }
    }

    if (copy.flags & TileFlags::IS_COLLIDABLE) {
      b2DestroyShape(copy.shapeDef, true);
    }

    m_mainTiles.erase(pos);

    for (i16 x = pos.x; x < pos.x + copy.x.w; x++) {
      for (i16 y = pos.y; y < pos.y + copy.y.y; y++) {
        Tile& subtile = find({x, y});

        if (subtile.entity) {
          entity = subtile.entity;
        }

        m_tiles.erase({x, y});
      }
    }

    return entity;
  }

  Tile& find(const glm::i16vec2& pos) { return m_tiles.find(pos)->second; }

  const Tile& find(const glm::i16vec2& pos) const { return m_tiles.find(pos)->second; }

  bool contains(const glm::i16vec2& pos) const { return m_tiles.contains(pos); }

  static Vec2 getLocalTileCenter(const glm::i16vec2& tilePos, const glm::u16vec2& dim = {1, 1}) {
    return static_cast<glm::vec2>(tilePos) * tileSize + ((Vec2)dim * tileSize * 0.5f);
  }

  static glm::i16vec2 getNearestTile(const glm::vec2& localPos) {
    return glm::i16vec2(glm::floor(localPos / tileSize));
  }

  public:
  template <typename ItType>
  class Iterator {
    ItType m_it;

public:
    Iterator(ItType it) : m_it(it) {}

    const glm::i16vec2& operator*() const { return *m_it; }

    Iterator<ItType>& operator++() {
      m_it = ++m_it;
      return *this;
    }

    Iterator<ItType>& operator--() {
      m_it = --m_it;
      return *this;
    }

    bool operator!=(const Iterator<ItType>& other) const { return m_it != other.m_it; }
  };

  using MapType = Set<glm::i16vec2>;
  Iterator<MapType::iterator> begin() { return m_mainTiles.begin(); }
  Iterator<MapType::iterator> end() { return m_mainTiles.end(); }
  Iterator<MapType::const_iterator> begin() const { return m_mainTiles.begin(); }
  Iterator<MapType::const_iterator> end() const { return m_mainTiles.end(); }

  protected:
  u32 m_layer = 0;
  OpenMap<glm::i16vec2, Tile> m_tiles;
  Set<glm::i16vec2> m_mainTiles;
};

enum TilemapLayers : u32 { TilemapWorldLayer, TilemapPlayerLayer };

struct TilemapComponent {
  TilemapComponent() {
    for (u32 i = 0; i < m_tilemaps.size(); i++)
      m_tilemaps[i].m_layer = i;
  }

  void addLayer() {
    assert(m_layerCount + 1 <= 3);

    m_layerCount += 1;
  }

  Tilemap& getTilemap(u32 layer) {
    assert(layer <= m_layerCount);

    return m_tilemaps[layer];
  }

  Tilemap& getBottomtilemap() { return m_tilemaps[0]; }

  Tilemap& getToptilemap() { return m_tilemaps[m_layerCount - 1]; }

  // returns the tilemap thats top most and contains pos
  u32 getTopTilemapWith(const glm::u16vec2& pos) {
    for (u32 i = m_layerCount; i != 0; i--) {
      if (m_tilemaps[i - 1].contains(pos))
        return i - 1;
    }

    return UINT32_MAX;
  }

  u32 getTilemapCount() const { return m_layerCount; }

  private:
  u32 m_layerCount = 2;
  std::array<Tilemap, 3> m_tilemaps;
};

struct DestroyTileOnEntityRemovalTag {};
