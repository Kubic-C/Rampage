#include "tilemap.hpp"

RAMPAGE_START

void MultiTileComponent::serialize(capnp::MessageBuilder& builder, Ref component) {
  auto multiTileBuilder = builder.initRoot<Schema::MultiTileComponent>();
  auto self = component.cast<MultiTileComponent>();
  
  // Serialize occupiedPositions
  auto positionsBuilder = multiTileBuilder.initOccupiedPositions((u32)self->occupiedPositions.size());
  for (size_t i = 0; i < self->occupiedPositions.size(); ++i) {
    const auto& pos = self->occupiedPositions[i];
    positionsBuilder[i].setX((i32)pos.x);
    positionsBuilder[i].setY((i32)pos.y);
    positionsBuilder[i].setZ(0);
  }
}

void MultiTileComponent::deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component) {
  auto multiTileReader = reader.getRoot<Schema::MultiTileComponent>();
  auto self = component.cast<MultiTileComponent>();
  
  // Deserialize occupiedPositions
  self->occupiedPositions.clear();
  const auto positionsReader = multiTileReader.getOccupiedPositions();
  for (auto posReader : positionsReader) {
    glm::ivec2 pos((i32)posReader.getX(), (i32)posReader.getY());
    self->occupiedPositions.push_back(pos);
  }
}

void MultiTileComponent::fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& jsonValue) {
  auto self = component.cast<MultiTileComponent>();
  auto compJson = jsonValue.as<JSchema::MultiTileComponent>();
  
  // Parse occupiedPositions array
  if (compJson->hasOccupiedPositions()) {
    self->occupiedPositions.clear();
    for (const auto& posJson : compJson->getOccupiedPositions()) {
      glm::ivec2 pos(0);
      if (posJson.hasX())
        pos.x = posJson.getX();
      if (posJson.hasY())
        pos.y = posJson.getY();
      self->occupiedPositions.push_back(pos);
    }
  }
}

void TileComponent::serialize(capnp::MessageBuilder& builder, Ref component) {
  auto tileBuilder = builder.initRoot<Schema::TileComponent>();
  auto self = component.cast<TileComponent>();

  // Serialize collidable
  tileBuilder.setCollidable(self->collidable);

  // Serialize layer
  tileBuilder.setLayer(static_cast<u8>(self->layer));
}

void TileComponent::deserialize(capnp::MessageReader& reader, const IdMapper& idMapper, Ref component) {
  auto tileReader = reader.getRoot<Schema::TileComponent>();
  auto self = component.cast<TileComponent>();

  // Deserialize collidable
  self->collidable = tileReader.getCollidable();

  // Deserialize layer
  self->layer = static_cast<WorldLayer>(tileReader.getLayer());
}

void TileComponent::fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& jsonValue) {
  auto self = component.cast<TileComponent>();
  auto compJson = jsonValue.as<JSchema::TileComponent>();


  // Parse collidable if provided
  if (compJson->hasCollidable()) {
    self->collidable = compJson->getCollidable();
  }

  // Parse layer if provided (string to enum conversion)
  if (compJson->hasLayer()) {
    const std::string& layerName = compJson->getLayer();
    if (layerName == "Floor") self->layer = WorldLayer::Floor;
    else if (layerName == "Wall") self->layer = WorldLayer::Wall;
    else if (layerName == "Item") self->layer = WorldLayer::Item;
    else if (layerName == "Top") self->layer = WorldLayer::Top;
    else self->layer = WorldLayer::Floor;
  }
}

void UniqueTileComponent::serialize(capnp::MessageBuilder& builder, Ref component) {
  auto tileBuilder = builder.initRoot<Schema::UniqueTileComponent>();
  auto self = component.cast<UniqueTileComponent>();

  tileBuilder.setParent(self->entity);
  auto reader = tileBuilder.getTilePos();
  reader.setX(self->worldPos.x);
  reader.setY(self->worldPos.y);
  reader.setZ(static_cast<int>(self->layer));
}

void UniqueTileComponent::deserialize(capnp::MessageReader& reader, const IdMapper& idMapper, Ref component) {
  auto tileReader = reader.getRoot<Schema::UniqueTileComponent>();
  auto self = component.cast<UniqueTileComponent>();

  self->entity = idMapper.resolve(tileReader.getParent());
  auto posReader = tileReader.getTilePos();
  self->worldPos.x = posReader.getX();
  self->worldPos.y = posReader.getY();
  self->layer = static_cast<WorldLayer>(posReader.getZ());
}

void TilemapComponent::serialize(capnp::MessageBuilder& builder, Ref component) {
  auto tilemapBuilder = builder.initRoot<Schema::TilemapComponent>();
  auto self = component.cast<TilemapComponent>();
  
  // Serialize tiles map as list of TileEntry key-value pairs
  auto tilesBuilder = tilemapBuilder.initTiles((u32)self->getSize());
  
  u32 i = 0;
  for (size_t l = 0; l < TilemapComponent::LayerCount; ++l) {
    for (auto& [chunkPos, chunk] : self->m_layers[static_cast<size_t>(l)]) {
      for(size_t index = 0; index < TileChunk::chunkSize * TileChunk::chunkSize; ++index) {
        if(!chunk.hasTile(TileChunk::convertFromIndex(index)))
          continue;
        const Tile& tile = chunk.getTile(TileChunk::convertFromIndex(index));
        glm::ivec2 pos = getTilePosFromChunk(chunkPos, TileChunk::convertFromIndex(index));

        auto tileEntryBuilder = tilesBuilder[i++];
        auto posBuilder = tileEntryBuilder.getPos();
        posBuilder.setX((i32)pos.x);
        posBuilder.setY((i32)pos.y);
        posBuilder.setZ((i32)l);
        tileEntryBuilder.setEntityId(tile.tileId);
        auto rootBuilder = tileEntryBuilder.getRoot();
        rootBuilder.setX((i32)tile.root.x);
        rootBuilder.setY((i32)tile.root.y);
        tileEntryBuilder.setEnabled(tile.enabled);
      }
    }
  }
}

Vec2 getCenterOfShape(b2ShapeId shapeId) {
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

void TilemapComponent::deserialize(capnp::MessageReader& reader, const IdMapper& idMapper, Ref component) {
  auto tilemapReader = reader.getRoot<Schema::TilemapComponent>();
  auto world = component.getWorld();
  auto self = component.cast<TilemapComponent>();
  auto body = self.getEntity().get<BodyComponent>()->id;
  
  // Clear all layers
  for (size_t l = 0; l < TilemapComponent::LayerCount; ++l)
    self->m_layers[static_cast<size_t>(l)].clear();

  // Deserialize tiles list back into layered maps
  const auto tilesReader = tilemapReader.getTiles();
  for (auto tileEntryReader : tilesReader) {
    const auto posReader = tileEntryReader.getPos();
    const auto rootReader = tileEntryReader.getRoot();
    glm::ivec2 pos(posReader.getX(), posReader.getY());
    glm::ivec2 root(rootReader.getX(), rootReader.getY());
    WorldLayer layer = static_cast<WorldLayer>(posReader.getZ());
    EntityId entityId = idMapper.resolve(tileEntryReader.getEntityId());
    self->setTile(component.getEntity(), layer, pos, root, entityId);
    self->getTile(layer, pos).enabled = tileEntryReader.getEnabled();
  }

  u32 shapeCount = b2Body_GetShapeCount(body);
  std::vector<b2ShapeId> shapeIds(shapeCount); // to keep track of shapeIds for cleanup if needed
  b2Body_GetShapes(body, shapeIds.data(), shapeCount);
  for (u32 i = 0; i < shapeCount; i++) {
    b2ShapeId shapeId = shapeIds[i];
    Vec2 pos = getCenterOfShape(shapeId); // Get the position of the shape's center
    glm::ivec2 tilePos = getNearestTile(pos);
    for (size_t l = 0; l < TilemapComponent::LayerCount; ++l) {
      WorldLayer layer = static_cast<WorldLayer>(l);
      if (!self->containsTile(layer, tilePos)) 
        continue;

      Tile& tile = self->getTile(layer, tilePos); 
      tile.shapeId = shapeId;
      b2Shape_SetUserData(shapeId, &tile.ref); // Associate shape with tilemap entity for collision handling
    }
  }
}

void TilemapComponent::fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& compJson) {
  // TilemapComponent is not part of the JSON schema variant;
  // tiles are populated via deserialization, not fromJson.
}

void TilemapComponent::copy(Ref src, Ref dst) {
  auto world = src.getWorld();
  auto srcTilemap = src.cast<TilemapComponent>();
  auto dstTilemap = dst.cast<TilemapComponent>();

  for (size_t l = 0; l < TilemapComponent::LayerCount; ++l) {
    for (auto& [chunkPos, chunk] : srcTilemap->m_layers[l]) {
      for(size_t index = 0; index < TileChunk::chunkSize * TileChunk::chunkSize; ++index) {
        if(!chunk.hasTile(TileChunk::convertFromIndex(index)))
          continue;
        Tile tile = chunk.getTile(TileChunk::convertFromIndex(index));
        glm::ivec2 pos = getTilePosFromChunk(chunkPos, TileChunk::convertFromIndex(index));
        auto tileEntity = world->getEntity(tile.tileId); 
        auto tileComp = tileEntity.get<TileComponent>();
        if (tileEntity.has<UniqueTileComponent>()) {
          tile.tileId = world->clone(tile.tileId);
        }

        dstTilemap->setTile(dst.getEntity(), static_cast<WorldLayer>(l), pos, tile.root, tile.tileId);
      }
    }
  }

  auto body = dst.getEntity().get<BodyComponent>()->id;
  u32 shapeCount = b2Body_GetShapeCount(body);
  std::vector<b2ShapeId> shapeIds(shapeCount); // to keep track of shapeIds for cleanup if needed
  b2Body_GetShapes(body, shapeIds.data(), shapeCount);
  for (u32 i = 0; i < shapeCount; i++) {
    b2ShapeId shapeId = shapeIds[i];
    Vec2 pos = getCenterOfShape(shapeId); // Get the position of the shape's center
    glm::ivec2 tilePos = getNearestTile(pos);
    for (size_t l = 0; l < TilemapComponent::LayerCount; ++l) {
      WorldLayer layer = static_cast<WorldLayer>(l);
      if (!dstTilemap->containsTile(layer, tilePos))
        continue;

      Tile& tile = dstTilemap->getTile(layer, tilePos);
      tile.shapeId = shapeId;
      b2Shape_SetUserData(shapeId, &tile.ref); // Associate shape with tilemap entity for collision handling
    }
  }
}

RAMPAGE_END