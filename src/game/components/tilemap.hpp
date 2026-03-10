#pragma once

#include "../../core/module.hpp"
#include "sprite.hpp"
#include "body.hpp"

RAMPAGE_START

constexpr glm::vec2 tileSize = glm::vec2(0.5f, 0.5f);

constexpr std::array<glm::ivec2, 4> tileDirectionPos = {
  glm::ivec2(1, 0), 
  glm::ivec2(0, -1), 
  glm::ivec2(-1, 0), 
  glm::ivec2(0, 1)
};

enum TileDirection: u8 {
  Right = 0,
  Down = 1,
  Left = 2,
  Up = 3
};

inline TileDirection getOppositeDirection(TileDirection dir) {
  switch(dir) {
    case TileDirection::Up: return TileDirection::Down;
    case TileDirection::Right: return TileDirection::Left;
    case TileDirection::Down: return TileDirection::Up;
    case TileDirection::Left: return TileDirection::Right;
  }
}

inline TileDirection getRotateTileDirection(TileDirection dir, int steps) {
  return static_cast<TileDirection>((static_cast<int>(dir) + steps) & 3);
}

inline float getTileDirectionToRadians(TileDirection dir) {
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
  glm::vec2 calculateCentroid(const glm::ivec2& offset) const {
    glm::vec2 sum(0.0f);
    for (const auto& pos : occupiedPositions) {
      sum += glm::vec2(getLocalTileCenter(pos + offset));
    }
    return sum / static_cast<float>(occupiedPositions.size());
  }

  // (0, 0) is the root position
  std::vector<glm::ivec2> occupiedPositions;  // All grid cells this entity occupies
};

struct TileComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component);
  static void deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component);
  static void fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& compJson);

  // If FALSE: every reference to this tile component is the same entity.
  // If TRUE: every reference to this tile component is WILL be a different entity.
  bool collidable = true;
  WorldLayer layer = WorldLayer::Floor;
};

class TilemapManager;

struct TileRef : EntityBox2dUserData {
  TileRef()
    : EntityBox2dUserData(UserDataType::Tile) {}

  EntityId tilemap = NullEntityId; // Entity that this tile belongs to
  glm::ivec2 worldPos = {INT_MAX, INT_MAX};
  WorldLayer layer = WorldLayer::Invalid;
};

struct UniqueTileComponent : TileRef, JsonableTag {
  static void serialize(capnp::MessageBuilder& builder, Ref component);
  static void deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component);
};

struct Tile {
  glm::ivec2 root = {0, 0}; // position of the root tile in tile coordinates 
  EntityId tileId = NullEntityId;
  b2ShapeId shapeId = b2_nullShapeId; 
  TileDirection rotation = TileDirection::Right;
  bool enabled = true;

  // For Shape's user data
  TileRef ref;
};

class TileChunk {
public:
  static constexpr int chunkSize = 64;
  static constexpr glm::vec2 chunkTileSize = tileSize * (float)64;
  using TilesList = std::array<Tile, chunkSize * chunkSize>;

  static size_t convertToIndex(const glm::ivec2& localPos) {
    return localPos.y * chunkSize + localPos.x;
  }

  static glm::ivec2 convertFromIndex(size_t index) {
    glm::ivec2 pos;
    pos.x = (int)index % chunkSize;
    pos.y = (int)index / chunkSize;
    return pos;
  }

  bool insertTile(glm::ivec2 localPos, EntityId entity, const glm::ivec2& root, const TileRef& ref) {
    return insertTile(convertToIndex(localPos), entity, root, ref);
  }

  bool removeTile(glm::ivec2 localPos) {
    return removeTile(convertToIndex(localPos));
  }

  Tile& getTile(glm::ivec2 localPos) {
    return getTile(convertToIndex(localPos));
  }

  Tile getTile(glm::ivec2 localPos) const {
    return getTile(convertToIndex(localPos));
  }

  bool hasTile(const glm::ivec2& pos) const {
    return hasTile(convertToIndex(pos));
  }

  bool insertTile(size_t index, EntityId entity, const glm::ivec2& root, const TileRef& tileRef) {
    if (hasTile(index)) {
      return false; // Tile already exists at this position
    }

    curSize++;
    tiles[index].tileId = entity;
    tiles[index].ref = tileRef;
    tiles[index].root = root;
    return true;
  }

  bool removeTile(size_t index) {
    Tile& tile = tiles[index];
    if (tile.tileId == NullEntityId)
      return false;
    tile = Tile();
    curSize--;
    return true;
  }

  Tile& getTile(size_t index) {
    Tile& tile = tiles[index];
    if (tile.ref.entity == NullEntityId)
      throw std::out_of_range("No tile at local position " + std::to_string(index));
    return tile;
  }

  const Tile& getTile(size_t index) const {
    const Tile& tile = tiles[index];
    if (tile.ref.entity == NullEntityId)
      throw std::out_of_range("No tile at local position " + std::to_string(index));
    return tile;
  }

  bool hasTile(size_t index) const {
    return tiles[index].ref.entity != NullEntityId;
  }

  size_t getSize() const {
    return curSize;
  }

private:
  size_t curSize = 0; // number of non-empty tiles in chunk
  TilesList tiles; // Entity IDs
};

inline Vec2 getLocalChunkCenter(const glm::ivec2& chunkCoords) {
  return static_cast<glm::vec2>(chunkCoords) * static_cast<float>(TileChunk::chunkSize) * tileSize +
      static_cast<float>(TileChunk::chunkSize) * tileSize * 0.5f;
}

inline Vec2 getLocalChunkCorner(const glm::ivec2& chunkCoords) {
  return static_cast<glm::vec2>(chunkCoords) * static_cast<float>(TileChunk::chunkSize) * tileSize;
}

inline glm::ivec2 getNearestChunkWorldPos(const glm::vec2& worldPos) {
  return glm::floor(worldPos / (tileSize * static_cast<float>(TileChunk::chunkSize)));
}

// Returns tilePos in relation to the chunk corner (local chunk coordinates)
inline glm::ivec2 getChunkLocalTilePos(const glm::ivec2& worldTilePos) {
  int cs = static_cast<int>(TileChunk::chunkSize); // safe cast (chunkSize is usually 16/32/64)
  glm::ivec2 mod = worldTilePos % cs; // component-wise
  return (mod + cs) % cs;
}

// Returns the chunk position that tilePos belongs to (chunk coordinates)
inline glm::ivec2 getChunkPos(const glm::ivec2& worldTilePos) {
  int cs = static_cast<int>(TileChunk::chunkSize);
  glm::ivec2 chunkPos;
  chunkPos.x = (worldTilePos.x >= 0) ? (worldTilePos.x / cs) : ((worldTilePos.x - cs + 1) / cs);
  chunkPos.y = (worldTilePos.y >= 0) ? (worldTilePos.y / cs) : ((worldTilePos.y - cs + 1) / cs);
  return chunkPos;
}
inline glm::ivec2 getTilePosFromChunk(const glm::ivec2& chunkCoords, const glm::ivec2& localTilePos) {
  return chunkCoords * static_cast<int>(TileChunk::chunkSize) + localTilePos;
}

struct TilemapComponent {
protected:
  friend class TilemapManager;
public:
  static constexpr size_t LayerCount = static_cast<size_t>(WorldLayer::Invalid);
  using ChunkLayerList = std::array<Map<glm::ivec2, TileChunk>, LayerCount>;

  static void serialize(capnp::MessageBuilder& builder, Ref component);
  static void deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component);
  static void fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& compJson);
  static void copy(Ref src, Ref dst);

  // Getters
  Tile& getTile(const TileRef& ref) {
    return getTile(ref.layer, ref.worldPos);
  }

  Tile getTile(const TileRef& ref) const {
    return getTile(ref.layer, ref.worldPos);
  }

  Tile getTile(WorldLayer layer, const glm::ivec2& pos) const {
    glm::ivec2 chunkPos = getChunkPos(pos);
    glm::ivec2 localTilePos = getChunkLocalTilePos(pos);

    const auto& chunk = m_layers[static_cast<size_t>(layer)].at(chunkPos);
    return chunk.getTile(localTilePos);
  }

  Tile& getTile(WorldLayer layer, const glm::ivec2& pos) {
    glm::ivec2 chunkPos = getChunkPos(pos);
    glm::ivec2 localTilePos = getChunkLocalTilePos(pos);

    auto& chunk = m_layers[static_cast<size_t>(layer)].at(chunkPos);
    return chunk.getTile(localTilePos);
  }

  bool containsTile(WorldLayer wLayer, const glm::ivec2& pos) const {
    size_t layer = static_cast<size_t>(wLayer);
    glm::ivec2 chunkPos = getChunkPos(pos);
    glm::ivec2 localTilePos = getChunkLocalTilePos(pos);

    if(!m_layers[layer].contains(chunkPos))
      return false;
    const auto& chunk = m_layers[layer].at(chunkPos);
    return chunk.hasTile(localTilePos);
  }

  bool containsTileAtAnyLayer(const glm::ivec2& pos) const {
    for (size_t l = 0; l < LayerCount; ++l)
      if (containsTile(static_cast<WorldLayer>(l), pos)) return true;
    return false;
  }

  WorldLayer getLayerOfTopTile(const glm::ivec2& pos) const {
    for (size_t l = LayerCount; l-- > 0;) {
      if (containsTile(static_cast<WorldLayer>(l), pos)) return static_cast<WorldLayer>(l);
    }
    return WorldLayer::Invalid;
  }

  bool isEmpty() const {
    for (const auto& layer : m_layers)
      if (!layer.empty()) return false;
    return true;
  }

  size_t getSize() const {
    size_t totalSize = 0;
    for (const auto& layer : m_layers) {
      for (const auto& chunkPair : layer) {
        totalSize += chunkPair.second.getSize();
      }
    }
    return totalSize;
  }

  const ChunkLayerList& getChunkLayers() const {
    return m_layers;
  }

protected:

  // Setters
  void setTile(EntityId tilemap, WorldLayer wLayer, glm::ivec2 pos, glm::ivec2 root, EntityId id) {
    size_t layer = static_cast<size_t>(wLayer);
    glm::ivec2 chunkPos = getChunkPos(pos);
    glm::ivec2 localTilePos = getChunkLocalTilePos(pos);

    TileRef tileRef;
    tileRef.tilemap = tilemap;
    tileRef.entity = id;
    tileRef.layer = wLayer;
    tileRef.worldPos = pos;
    assert(static_cast<int>(wLayer) <= 3);
    m_layers[layer][chunkPos].insertTile(localTilePos, id, root, tileRef);
  }

  bool insertTile(EntityId tilemap, WorldLayer wLayer, glm::ivec2 pos, glm::ivec2 root, EntityId id) {
    auto& layerMap = m_layers[static_cast<size_t>(wLayer)];
    glm::ivec2 chunkPos = getChunkPos(pos);
    glm::ivec2 localTilePos = getChunkLocalTilePos(pos);

    if (layerMap[chunkPos].hasTile(localTilePos)) {
      return false; // Tile already exists at this position
    }

    TileRef tileRef;
    tileRef.tilemap = tilemap;
    tileRef.entity = id;
    tileRef.layer = wLayer;
    tileRef.worldPos = pos;
    assert(static_cast<int>(wLayer) <= 3);
    layerMap[chunkPos].insertTile(localTilePos, id, root, tileRef);
    return true;
  }

  void removeTile(WorldLayer wLayer, glm::ivec2 pos) {
    size_t layer = static_cast<size_t>(wLayer);
    glm::ivec2 chunkPos = getChunkPos(pos);
    glm::ivec2 localTilePos = getChunkLocalTilePos(pos);

    if (m_layers[layer].contains(chunkPos)) {
      m_layers[layer][chunkPos].removeTile(localTilePos);

      if (m_layers[layer][chunkPos].getSize() == 0) {
        m_layers[layer].erase(chunkPos);                      // optional but highly recommended
      } 
    }
  }

private:
  ChunkLayerList m_layers;
};

struct _GetShapeEntityContext {
  IWorldPtr world;
  std::vector<TileRef> tilesAtPlacement;
};

inline float _getShapeEntity(b2ShapeId shapeId, b2Vec2 point, b2Vec2 normal, float fraction, void* context) {
  _GetShapeEntityContext* ctx = reinterpret_cast<_GetShapeEntityContext*>(context);
  IWorldPtr world = ctx->world;
  auto& tilesAtPlacement = ctx->tilesAtPlacement;

  EntityBox2dUserData* entityUserData = getEntityBox2dUserData(shapeId);
  if (!entityUserData || entityUserData->type != UserDataType::Tile)
    return -1.0f;

  TileRef* tileData = (TileRef*)entityUserData;
  tilesAtPlacement.push_back(*tileData);

  return -1.0f;
}

inline TileRef getTileAtPos(IWorldPtr world, Vec2 worldPos) {
  b2WorldId worldId = world->getContext<b2WorldId>();

  _GetShapeEntityContext context;
  context.world = world;
  b2QueryFilter filter;
  filter.categoryBits = Static;
  filter.maskBits = Static | Friendly;
  b2World_CastRay(worldId, worldPos, {0}, filter, &_getShapeEntity, &context);

  for (const auto& tileRef : context.tilesAtPlacement) {
    return tileRef; // just return the first available tile position, we don't care about layers here
  }

  return TileRef(); // null entity
}

inline TileRef getTopTileAtPos(IWorldPtr world, Vec2 worldPos) {
  b2WorldId worldId = world->getContext<b2WorldId>();

  _GetShapeEntityContext context;
  context.world = world;
  b2QueryFilter filter;
  filter.categoryBits = Static;
  filter.maskBits = Static | Friendly;
  b2World_CastRay(worldId, worldPos, {0}, filter, &_getShapeEntity, &context);

  size_t retIndex = 0;
  WorldLayer topLayer = WorldLayer::Floor;
  for (size_t i = 0; i < context.tilesAtPlacement.size(); i++) {
    if (context.tilesAtPlacement[i].layer >= topLayer) {
      topLayer = context.tilesAtPlacement[i].layer;
      retIndex = i;
    }
  }

  if(context.tilesAtPlacement.empty())
    return TileRef(); // null entity
  else
    return context.tilesAtPlacement[retIndex];
}

// Like getTileAtPos but prefers tiles that have a specific component.
// Falls back to the first TileComponent entity if none have the component.
template<typename TComp>
inline TileRef getTileWithComponentAtPos(IWorldPtr world, Vec2 worldPos) {
  b2WorldId worldId = world->getContext<b2WorldId>();

  _GetShapeEntityContext context;
  context.world = world;
  b2QueryFilter filter;
  filter.categoryBits = Static;
  filter.maskBits = Static | Friendly;
  b2World_CastRay(worldId, worldPos, {0}, filter, &_getShapeEntity, &context);

  for (size_t i = 0; i < context.tilesAtPlacement.size(); i++) {
    TileRef& tileRef = context.tilesAtPlacement[i];
    EntityPtr tilemapEntity = world->getEntity(tileRef.tilemap);
    auto tilemap = tilemapEntity.get<TilemapComponent>();
    EntityPtr tileEntity = world->getEntity(tilemap->getTile(tileRef.layer, tileRef.worldPos).tileId);

    if (tileEntity.has<TComp>())
      return tileRef;
  }
  return TileRef();
}

RAMPAGE_END
