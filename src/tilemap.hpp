#pragma once

#include "tilePrefabs.hpp"
#include "transform.hpp"

template<>
struct boost::hash<glm::i16vec2> {
  size_t operator()(const glm::i16vec2& pos) const {
    constexpr u16 prime1 = 65521;
    constexpr u16 prime2 = 57149;

    return (pos.x ^ prime1 * 0x5555) ^ (pos.y ^ prime2 * 0x5555);
  }
};

struct TileBoundComponent {
  glm::i16vec2 pos;
  EntityId parent;
};

struct TilemapComponent {
  // Ignores shapeId
  bool insert(EntityWorld& world, b2BodyId body, const glm::i16vec2& pos, EntityId parent, const TileDef& clone) {
    return insert(world, body, pos, parent, clone.entity, clone.flags, { clone.width, clone.height });
  }

  bool insert(EntityWorld& world, b2BodyId body, const glm::i16vec2& pos, EntityId parent = 0, EntityId entity = 0, u8 tileFlags = TileFlags::IS_COLLIDABLE, const glm::i16vec2& dim = { 1, 1 }, const b2ShapeDef& shapeDef = b2DefaultShapeDef()) {
    if (contains(pos))
      return false;

    if (dim.x != 1 || dim.y != 1)
      tileFlags |= TileFlags::IS_MULTI_TILE;

    Tile tile;
    tile.entity = entity;
    tile.flags = tileFlags | TileFlags::IS_MAIN_TILE;
    tile.width = dim.x;
    tile.height = dim.y;

    if (entity) {
      Entity e = world.get(entity);
      e.add<TileBoundComponent>();
      TileBoundComponent& tileBound = e.get<TileBoundComponent>();
      tileBound.parent = parent;
      tileBound.pos = pos;
    }

    if (tile.flags & TileFlags::IS_MULTI_TILE) {
      const glm::i16vec2 startPos = pos;
      const glm::i16vec2 endPos = pos + dim;

      Tile subTile;
      subTile.entity = NullEntityId;
      subTile.flags = tileFlags & ~TileFlags::IS_MAIN_TILE;
      subTile.posx = pos.x;
      subTile.posy = pos.y;
      subTile.shapeDef = b2_nullShapeId;

      for (i16 y = startPos.y; y < endPos.y; y++) {
        for (i16 x = startPos.x; x < endPos.x; x++) {
          if (contains({ x, y }))
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
      b2Polygon tilePolygon = b2MakeOffsetBox(fdim.x * tileSize.x * 0.5f, fdim.y * tileSize.y * 0.5f, tilePos, Rot(0.0f));
      tile.shapeDef = b2CreatePolygonShape(body, &shapeDef, &tilePolygon);
    }
    else {
      tile.shapeDef = b2_nullShapeId;
    }

    assert(!m_tiles.contains(pos));
    m_tiles.insert(std::make_pair(pos, tile));

    return true;
  }
  

  // !ATTENTION! this does not destroy the entity located with the tile
  EntityId erase(const glm::i16vec2& pos) {
    Tile copy = find(pos);
    EntityId entity = 0;

    if (copy.flags & TileFlags::IS_MULTI_TILE) {
      if (!(copy.flags & TileFlags::IS_MAIN_TILE)) {
        return erase(glm::i16vec2(copy.posx, copy.posy));
      }
    }

    if (copy.flags & TileFlags::IS_COLLIDABLE) {
      b2DestroyShape(copy.shapeDef, true);
    }

    for (i16 x = pos.x; x < pos.x + copy.width; x++) {
      for (i16 y = pos.y; y < pos.y + copy.height; y++) {
        Tile& subtile = find({x, y});

        if (subtile.entity) {
          entity = subtile.entity;
        }

        m_tiles.erase({ x, y });
      }
    }

    return entity;
  }

  Tile& find(const glm::i16vec2& pos) {
    return m_tiles.find(pos)->second;
  }

  const Tile& find(const glm::i16vec2& pos) const {
    return m_tiles.find(pos)->second;
  }

  bool contains(const glm::i16vec2& pos) const {
    return m_tiles.contains(pos);
  }

  Vec2 getLocalTileCenter(const glm::i16vec2& tilePos, const Vec2& dim = { 1, 1 }) const {
    return (glm::vec2)tilePos * tileSize + (dim * tileSize * 0.5f);
  }

  glm::i16vec2 getNearestTile(const glm::vec2& localPos) const {
    return glm::i16vec2(glm::floor(localPos / tileSize));
  }

public:
  template<typename ItType>
  class Iterator {
    ItType m_it;
  public:
    Iterator(ItType it)
      : m_it(it) {
    }

    const glm::i16vec2& operator*() const {
      return m_it->first;
    }

    Iterator<ItType>& operator++() {
      m_it = ++m_it;
      return *this;
    }

    Iterator<ItType>& operator--() {
      m_it = --m_it;
      return *this;
    }

    bool operator!=(const Iterator<ItType>& other) const {
      return m_it != other.m_it;
    }
  };

  using MapType = OpenMap<glm::i16vec2, Tile>;
  Iterator<MapType::iterator> begin() { return m_tiles.begin(); }
  Iterator<MapType::iterator> end() { return m_tiles.end(); }
  Iterator<MapType::const_iterator> begin() const { return m_tiles.begin(); }
  Iterator<MapType::const_iterator> end() const { return m_tiles.end(); }

  OpenMap<glm::i16vec2, Tile> m_tiles;
};

class TilemapModule : public Module {
public:
  TilemapModule(EntityWorld& world)
    : m_calcTransforms(world.system(world.set<TransformComponent, TileBoundComponent>(), &updateTileBoundTransforms)) {
    world.component<TilemapComponent>();
    world.component<TileBoundComponent>();
  }

  static void updateTileBoundTransforms(Entity e, float dt) {
    TransformComponent& transform = e.get<TransformComponent>();
    TileBoundComponent& tileBound = e.get<TileBoundComponent>();

    Entity parent = e.world().get(tileBound.parent);
    TransformComponent& parentTransform = parent.get<TransformComponent>();
    TilemapComponent& parentTilemap = parent.get<TilemapComponent>();
    Tile& tile = parentTilemap.find(tileBound.pos);

    transform = Transform(parentTransform.getWorldPoint(parentTilemap.getLocalTileCenter(tileBound.pos, tile.size())), transform.rot);
  }

  void run(EntityWorld& world, float deltaTime) override {
    m_calcTransforms.run();
  }

private:
  System m_calcTransforms;
};