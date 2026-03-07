#pragma once

#include "../../core/module.hpp"
#include "sprite.hpp"
#include "body.hpp"

RAMPAGE_START

constexpr glm::vec2 tileSize = glm::vec2(0.5f, 0.5f);

constexpr std::array<glm::ivec2, 4> tileDirectionPos = {
  glm::ivec2(0, 1), 
  glm::ivec2(1, 0), 
  glm::ivec2(0, -1), 
  glm::ivec2(-1, 0)
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

inline TileDirection rotateTileDirection(TileDirection dir, int steps) {
  return static_cast<TileDirection>((static_cast<int>(dir) + steps) & 3);
}

inline float tileDirectionToRadians(TileDirection dir) {
  // Up=0, Right=-π/2, Down=π, Left=π/2
  static constexpr float angles[] = { 0.0f, -glm::half_pi<float>(), glm::pi<float>(), glm::half_pi<float>() };
  return angles[static_cast<u8>(dir)];
}

inline glm::ivec2 rotateTileOffset(glm::ivec2 offset, TileDirection rotation) {
  switch (rotation) {
    case TileDirection::Up:    return offset;
    case TileDirection::Right: return { offset.y, -offset.x };
    case TileDirection::Down:  return { -offset.x, -offset.y };
    case TileDirection::Left:  return { -offset.y, offset.x };
  }
  return offset;
}

inline Vec2 getLocalTileCenter(const glm::ivec2& tilePos) {
  return static_cast<glm::vec2>(tilePos) * tileSize + tileSize * 0.5f;
}

inline glm::ivec2 getNearestTile(const glm::vec2& localPos) {
  return glm::ivec2(glm::floor(localPos / tileSize));
}

struct MultiTileComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component);
  static void deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component);
  static void fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& compJson);

  // Returns the LOCAL NON TILE COORDINATE of the multile
  glm::vec2 calculateCentroid() const {
    glm::vec2 sum(0.0f);
    for (const auto& pos : occupiedPositions) {
      sum += glm::vec2(getLocalTileCenter(pos));
    }
    return sum / static_cast<float>(occupiedPositions.size());
  }

  std::vector<glm::ivec2> occupiedPositions;  // All grid cells this entity occupies
  glm::ivec2 anchorPos;  // Primary position
};

struct TileComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component);
  static void deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component);
  static void fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& compJson);

  bool collidable = true;
  b2ShapeId shapeId;
  glm::ivec2 pos = {0, 0};
  WorldLayer layer = WorldLayer::Floor;
  TileDirection rotation = TileDirection::Up;
  EntityId parent = 0;
  b2SurfaceMaterial material = b2DefaultSurfaceMaterial();
};

class TilemapManager;

struct TilemapComponent {
protected:
  friend class TilemapManager;
public:

  static constexpr size_t LayerCount = static_cast<size_t>(WorldLayer::Invalid);

  static void serialize(capnp::MessageBuilder& builder, Ref component);
  static void deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component);
  static void fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& compJson);
  static void copy(Ref src, Ref dst);

  // Getters
  EntityId getTile(WorldLayer layer, glm::ivec2 pos) const {
    return m_layers[static_cast<size_t>(layer)].at(pos);
  }

  bool containsTile(WorldLayer layer, glm::ivec2 pos) const {
    return m_layers[static_cast<size_t>(layer)].contains(pos);
  }

  bool containsTileAtAnyLayer(glm::ivec2 pos) const {
    for (size_t l = 0; l < LayerCount; ++l)
      if (m_layers[l].contains(pos)) return true;
    return false;
  }

  WorldLayer getLayerOfTopTile(glm::ivec2 pos) const {
    for (size_t l = 0; l < LayerCount; ++l)
      if (m_layers[l].contains(pos)) return static_cast<WorldLayer>(l);
    return WorldLayer::Invalid;
  }

  const OpenMap<glm::ivec2, EntityId>& getLayer(WorldLayer layer) const {
    return m_layers[static_cast<size_t>(layer)];
  }

  OpenMap<glm::ivec2, EntityId>& getLayer(WorldLayer layer) {
    return m_layers[static_cast<size_t>(layer)];
  }

  bool empty() const {
    for (const auto& layer : m_layers)
      if (!layer.empty()) return false;
    return true;
  }

  size_t size() const {
    size_t total = 0;
    for (const auto& layer : m_layers)
      total += layer.size();
    return total;
  }

protected:

  // Setters
  void setTile(WorldLayer layer, glm::ivec2 pos, EntityId id) {
    m_layers[static_cast<size_t>(layer)][pos] = id;
  }

  bool insertTile(WorldLayer layer, glm::ivec2 pos, EntityId id) {
    auto& layerMap = m_layers[static_cast<size_t>(layer)];
    if (layerMap.contains(pos)) return false;
    layerMap[pos] = id;
    return true;
  }

  void removeTile(WorldLayer layer, glm::ivec2 pos) {
    m_layers[static_cast<size_t>(layer)].erase(pos);
  }

private:
  std::array<OpenMap<glm::ivec2, EntityId>, LayerCount> m_layers;
};

inline Vec2 getWorldTilePosition(EntityPtr tileEntity) {
  auto tileComp = tileEntity.get<TileComponent>();
  EntityPtr tilemap = tileEntity.world()->getEntity(tileEntity.get<TileComponent>()->parent);
  auto bodyComp = tilemap.get<BodyComponent>();

  Vec2 localPos;
  if(tileEntity.has<MultiTileComponent>()) {
    auto multiTile = tileEntity.get<MultiTileComponent>();
    localPos = multiTile->calculateCentroid();
  } else 
    localPos = getLocalTileCenter(tileComp->pos);

  Vec2 worldPos = b2Body_GetWorldPoint(bodyComp->id, localPos);
  return worldPos;
}


inline float _getShapeEntity(b2ShapeId shapeId, b2Vec2 point, b2Vec2 normal, float fraction, void* context) {
  std::vector<EntityId>& entitiesAtPlacement = *(std::vector<EntityId>*)context;

  EntityId entityId = b2RawDataToEntity(b2Shape_GetUserData(shapeId));
  if(entityId != 0) {
    entitiesAtPlacement.push_back(entityId);
  }

  return -1.0f;
}

inline EntityPtr getTileAtPos(IWorldPtr world, Vec2 worldPos) {
  b2WorldId worldId = world->getContext<b2WorldId>();

  std::vector<EntityId> entitiesAtPlacement;
  b2QueryFilter filter;
  filter.categoryBits = Static;
  filter.maskBits = Static | Friendly;
  b2World_CastRay(worldId, worldPos, {0}, filter, &_getShapeEntity, &entitiesAtPlacement);

  for(EntityId entityId : entitiesAtPlacement) {
    EntityPtr entity = world->getEntity(entityId);
    if (entity.has<TileComponent>()) {
      return entity;
    }
  }

  return world->getEntity(0); // null entity
}

RAMPAGE_END
