#pragma once

#include "../../core/module.hpp"
#include "tile.hpp"
#include "body.hpp"

RAMPAGE_START

class TilemapComponent;

struct TileBoundComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component) {
    auto boundBuilder = builder.initRoot<Schema::TileBoundComponent>();
    auto bound = component.cast<TileBoundComponent>();

    boundBuilder.setLayer(bound->layer);
    boundBuilder.getPos().setX(bound->pos.x);
    boundBuilder.getPos().setY(bound->pos.y);
    boundBuilder.setParent(bound->parent);
  }

  static void deserialize(capnp::MessageReader& reader, Ref component) {
    auto boundReader = reader.getRoot<Schema::TileBoundComponent>();
    auto bound = component.cast<TileBoundComponent>();

    bound->layer = boundReader.getLayer();
    bound->pos.x = boundReader.getPos().getX();
    bound->pos.y = boundReader.getPos().getY();
    bound->parent = boundReader.getParent();
  }

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
      auto tileBound = e.get<TileBoundComponent>();
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
      subTile.shapeId = b2_nullShapeId;

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
      tile.shapeId = b2CreatePolygonShape(body, &copyShapeDef, &tilePolygon);
    } else {
      tile.shapeId = b2_nullShapeId;
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
      b2DestroyShape(copy.shapeId, true);
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

  Tile& find(const glm::i16vec2& pos) {
    return m_tiles.find(pos)->second;
  }

  const Tile& find(const glm::i16vec2& pos) const {
    return m_tiles.find(pos)->second;
  }

  bool contains(const glm::i16vec2& pos) const {
    return m_tiles.contains(pos);
  }

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

    const glm::i16vec2& operator*() const {
      return *m_it;
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

  using MapType = Set<glm::i16vec2>;
  Iterator<MapType::iterator> begin() {
    return m_mainTiles.begin();
  }
  Iterator<MapType::iterator> end() {
    return m_mainTiles.end();
  }
  Iterator<MapType::const_iterator> begin() const {
    return m_mainTiles.begin();
  }
  Iterator<MapType::const_iterator> end() const {
    return m_mainTiles.end();
  }

protected:
  u32 m_layer = 0;
  OpenMap<glm::i16vec2, Tile> m_tiles;
  Set<glm::i16vec2> m_mainTiles;
};

enum TilemapLayers : u32 { TilemapWorldLayer, TilemapPlayerLayer };

struct TilemapComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component) {
    auto compBuilder = builder.initRoot<Schema::TilemapComponent>();
    auto comp = component.cast<TilemapComponent>();

    auto listBuilder = compBuilder.initTilemaps(comp->m_tilemaps.size());
    for (size_t i = 0; i < comp->m_tilemaps.size(); i++) {
      auto& tm = comp->m_tilemaps[i];
      auto tmBuilder = listBuilder[i];
      tmBuilder.setLayer(tm.m_layer);

      auto tileListBuilder = tmBuilder.initTiles(tm.m_tiles.size());
      size_t j = 0;
      for (auto& [pos, tile] : tm.m_tiles) {
        auto tileBuilder = tileListBuilder[j++];
        tileBuilder.setEntity(tile.entity);
        tileBuilder.setFlags(tile.flags);
        tileBuilder.getGridPos().setX(pos.x);
        tileBuilder.getGridPos().setY(pos.y);
        tileBuilder.setSizeOrPosX(tile.x.x);
        tileBuilder.setSizeOrPosX(tile.y.y);
      }
    }
  }

  static Vec2 getCenterOfShape(b2ShapeId shapeId) {
    switch (b2Shape_GetType(shapeId))
    {
    case b2_circleShape: {
      b2Circle circle = b2Shape_GetCircle(shapeId);
      return Vec2(circle.center);
    }
    case b2_polygonShape: {
      b2Polygon poly = b2Shape_GetPolygon(shapeId);
      return Vec2(poly.centroid);
    }
    default:
      return Vec2(0.0f);
    }
  }

  static void deserialize(capnp::MessageReader& reader, Ref component) {
    auto compReader = reader.getRoot<Schema::TilemapComponent>();
    auto comp = component.cast<TilemapComponent>();

    b2BodyId body = component.getEntity().get<BodyComponent>()->id;

    OpenMap<glm::i16vec2, b2ShapeId> shapeMap; 
    u32 shapeCount = b2Body_GetShapeCount(body);
    std::vector<b2ShapeId> shapeIds(shapeCount); // to keep track of shapeIds for cleanup if needed
    b2Body_GetShapes(body, shapeIds.data(), shapeCount);
    for (u32 i = 0; i < shapeCount; i++) {
      b2ShapeId shapeId = shapeIds[i]; 
      Vec2 pos = getCenterOfShape(shapeId); // Get the position of the shape's center
      shapeMap.insert({Tilemap::getNearestTile(pos), shapeId});
    }

    const auto tmList = compReader.getTilemaps();
    for (auto tmReader : tmList) {
      Tilemap tm;
      tm.m_layer = tmReader.getLayer();

      const auto tilesList = tmReader.getTiles();
      for (auto tileReader : tilesList) {
        glm::i16vec2 pos(tileReader.getGridPos().getX(), tileReader.getGridPos().getY());
        Tile t;
        t.entity = tileReader.getEntity();
        t.flags = tileReader.getFlags();
        t.x.x = tileReader.getSizeOrPosX();
        t.y.y = tileReader.getSizeOrPosY();

        // Use spatial location of the tile to determine the corresponding shape
        // on the parent physics body.
        glm::i16vec2 localPos = Tilemap::getNearestTile(Tilemap::getLocalTileCenter(pos, {t.x.x, t.y.y}));
        auto shapeIt = shapeMap.find(localPos);
        if (shapeIt != shapeMap.end() && t.flags & TileFlags::IS_MAIN_TILE) {
          t.shapeId = shapeIt->second;
          shapeMap.erase(shapeIt); // Remove from map to prevent reuse
        } else {
          t.shapeId = b2_nullShapeId;
        }

        tm.m_tiles[pos] = t;
        if(t.flags & TileFlags::IS_MAIN_TILE) {
          tm.m_mainTiles.insert(pos); 
        }
      }

      comp->m_tilemaps[tm.m_layer] = std::move(tm);
    }
  }

  TilemapComponent() {
    for (u32 i = 0; i < m_tilemaps.size(); i++)
      m_tilemaps[i].m_layer = i;
  }

  Tilemap& getTilemap(u32 layer) {
    assert(layer <= m_layerCount);

    return m_tilemaps[layer];
  }

  Tilemap& getBottomtilemap() {
    return m_tilemaps[0];
  }

  Tilemap& getToptilemap() {
    return m_tilemaps[m_layerCount - 1];
  }

  // returns the tilemap thats top most and contains pos
  u32 getTopTilemapWith(const glm::u16vec2& pos) {
    for (u32 i = m_layerCount; i != 0; i--) {
      if (m_tilemaps[i - 1].contains(pos))
        return i - 1;
    }

    return UINT32_MAX;
  }

  u32 getTilemapCount() const {
    return m_layerCount;
  }

private:
  static constexpr u32 m_layerCount = 2;
  std::array<Tilemap, m_layerCount> m_tilemaps;
};

struct DestroyTileOnEntityRemovalTag : SerializableTag {};

RAMPAGE_END
