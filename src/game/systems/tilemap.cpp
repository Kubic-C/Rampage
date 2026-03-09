#include "tilemap.hpp"
#include "../module.hpp"
#include "../components/player.hpp"

RAMPAGE_START

struct PreviousMultiTileComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component) {
    auto multiTileBuilder = builder.initRoot<Schema::MultiTileComponent>();
    MultiTileComponent& self = component.cast<PreviousMultiTileComponent>()->mt;
    
    // Serialize occupiedPositions
    auto positionsBuilder = multiTileBuilder.initOccupiedPositions((u32)self.occupiedPositions.size());
    for (size_t i = 0; i < self.occupiedPositions.size(); ++i) {
      const auto& pos = self.occupiedPositions[i];
      positionsBuilder[i].setX((i32)pos.x);
      positionsBuilder[i].setY((i32)pos.y);
      positionsBuilder[i].setZ(0);
    }
    
    // Serialize anchorPos
    auto anchorPosBuilder = multiTileBuilder.getAnchorPos();
    anchorPosBuilder.setX((i32)self.anchorPos.x);
    anchorPosBuilder.setY((i32)self.anchorPos.y);
    anchorPosBuilder.setZ(0);
  }

  static void deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component) {
    auto multiTileReader = reader.getRoot<Schema::MultiTileComponent>();
    MultiTileComponent& self = component.cast<PreviousMultiTileComponent>()->mt;
    
    // Deserialize occupiedPositions
    self.occupiedPositions.clear();
    const auto positionsReader = multiTileReader.getOccupiedPositions();
    for (auto posReader : positionsReader) {
      glm::ivec2 pos((i32)posReader.getX(), (i32)posReader.getY());
      self.occupiedPositions.push_back(pos);
    }
    
    // Deserialize anchorPos
    const auto anchorPosReader = multiTileReader.getAnchorPos();
    self.anchorPos = glm::ivec2((i32)anchorPosReader.getX(), (i32)anchorPosReader.getY());
  }

  MultiTileComponent mt;
};

bool TilemapManager::canInsert(IWorldPtr world, EntityId tilemapEntityId, WorldLayer layer, glm::ivec2 gridPos, EntityId tileEntityId) {
  EntityPtr tilemapEntity = world->getEntity(tilemapEntityId);
  EntityPtr tileEntity = world->getEntity(tileEntityId);
  auto tilemap = tilemapEntity.get<TilemapComponent>();
  auto bodyComp = tilemapEntity.get<BodyComponent>();
  if(!tileEntity.has<TileComponent>()) 
    return false;
  auto tile = tileEntity.get<TileComponent>();

  // Insert single/multi tile in the tilemap

  // 1) (READONLY) Determine all occupied positions based on whether it's a multi-tile or single tile entity
  std::vector<std::pair<glm::ivec2, EntityId>> occupiedPositions;
  occupiedPositions.push_back({gridPos, tileEntityId}); // The first tile IS ALWAYS the anchor tile.
  if(tileEntity.has<MultiTileComponent>()) {
    // Read from multi-tile component to determine all occupied positions,
    // and add them to the tilemap as well with the same parent
    auto multiTile = tileEntity.get<MultiTileComponent>();
    for(const auto& pos : multiTile->occupiedPositions) {
      if(pos == multiTile->anchorPos)
        continue; // Skip the anchor tile since we already added it
      glm::ivec2 rotatedOffset = rotateTileOffset(pos - multiTile->anchorPos, tile->rotation);
      occupiedPositions.push_back({rotatedOffset + gridPos, NullEntityId});
    }
  }

  // 2) (READONLY) Failure cases; 
  //  - If there's an existing tile on this layer, operation fails
  //  - If tile already belongs to another tilemap, operation fails
  //  - If tile has existing joints, operation fails
  //  - If layer is invalid, operation fails
  if(layer == WorldLayer::Invalid)
    return false;
  for(auto& pos : occupiedPositions)
    if(tilemap->containsTile(layer, pos.first))
      return false;
  if(tile->parent != 0)
    return false;
  if(b2Shape_IsValid(tile->shapeId))
    return false;
 
  return true;
}

bool TilemapManager::insertTile(IWorldPtr world, EntityId tilemapEntityId, WorldLayer layer, glm::ivec2 gridPos, EntityId tileEntityId) {
  EntityPtr tilemapEntity = world->getEntity(tilemapEntityId);
  EntityPtr tileEntity = world->getEntity(tileEntityId);
  auto tilemap = tilemapEntity.get<TilemapComponent>();
  auto bodyComp = tilemapEntity.get<BodyComponent>();
  if(!tileEntity.has<TileComponent>()) 
    return false;
  auto tile = tileEntity.get<TileComponent>();

  // Insert single/multi tile in the tilemap

  // 1) (READONLY) Determine all occupied positions based on whether it's a multi-tile or single tile entity
  std::vector<std::pair<glm::ivec2, EntityId>> occupiedPositions;
  occupiedPositions.push_back({gridPos, tileEntityId}); // The first tile IS ALWAYS the anchor tile.
  if(tileEntity.has<MultiTileComponent>()) {
    // Read from multi-tile component to determine all occupied positions,
    // and add them to the tilemap as well with the same parent
    auto multiTile = tileEntity.get<MultiTileComponent>();
    for(const auto& pos : multiTile->occupiedPositions) {
      if(pos == multiTile->anchorPos)
        continue; // Skip the anchor tile since we already added it
      glm::ivec2 rotatedOffset = rotateTileOffset(pos - multiTile->anchorPos, tile->rotation);
      occupiedPositions.push_back({rotatedOffset + gridPos, NullEntityId});
    }
  }

  // 2) (READONLY) Failure cases; 
  //  - If there's an existing tile on this layer, operation fails
  //  - If tile already belongs to another tilemap, operation fails
  //  - If tile has existing joints, operation fails
  //  - If layer is invalid, operation fails
  if(layer == WorldLayer::Invalid)
    return false;
  for(auto& pos : occupiedPositions)
    if(tilemap->containsTile(layer, pos.first))
      return false;
  if(tile->parent != 0)
    return false;
  if(b2Shape_IsValid(tile->shapeId))
    return false;

  // 3) Update tilemap and tile data and create additional subtiles as needed.
  tile->pos = gridPos;
  tile->layer = layer;
  tile->parent = tilemapEntityId;
  tilemap->setTile(layer, gridPos, tileEntityId);
  for(auto&[pos, entity] : occupiedPositions) {
    if(pos == gridPos)
      continue; // Skip the anchor tile since we already added it
    EntityPtr subTile = world->create();
    subTile.add<TileComponent>();
    auto subTileComp = subTile.get<TileComponent>();
    subTileComp->parent = tileEntityId;
    subTileComp->material = tile->material;
    subTileComp->pos = pos;
    subTileComp->layer = layer;

    entity = subTile;
    tilemap->setTile(layer, pos, subTile);
  }
  if(tileEntity.has<MultiTileComponent>()) {
    auto multiTile = tileEntity.get<MultiTileComponent>();
    tileEntity.add<PreviousMultiTileComponent>();
    auto prevMultiTile = tileEntity.get<PreviousMultiTileComponent>();
    prevMultiTile->mt.anchorPos = multiTile->anchorPos; // Store previous state for undo/rollback purposes
    prevMultiTile->mt.occupiedPositions = multiTile->occupiedPositions;
    // Write back to multitile the updated positions
    multiTile->anchorPos = gridPos;
    multiTile->occupiedPositions.clear();
    for (const auto& pos : occupiedPositions) { 
      multiTile->occupiedPositions.push_back(pos.first);
    }
  }
    
  // 4) Create Box2D shapes for each tile and subtile
  for(const auto& [pos, entity] : occupiedPositions) {
    auto tileComp = world->getEntity(entity).get<TileComponent>();

    Vec2 tilePos = getLocalTileCenter(pos);
    b2Polygon tilePolygon =
        b2MakeOffsetBox(tileSize.x * 0.5f, tileSize.y * 0.5f, tilePos, Rot(0.0f));
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.userData = entityToB2Data(tileEntity.id());
    shapeDef.material = tile->material;
    shapeDef.updateBodyMass = false;
    shapeDef.isSensor = !tile->collidable;
    tileComp->shapeId = b2CreatePolygonShape(bodyComp->id, &shapeDef, &tilePolygon);
  }

  b2Body_ApplyMassFromShapes(*bodyComp);

  return true;
}

EntityPtr TilemapManager::removeTile(IWorldPtr world, EntityId tilemapEntityId, WorldLayer layer, glm::ivec2 gridPos, bool autoDestroyEntity) {
  EntityPtr tilemapEntity = world->getEntity(tilemapEntityId);
  auto tilemap = tilemapEntity.get<TilemapComponent>();

  if(!tilemap->containsTile(layer, gridPos))
    return world->getEntity(NullEntityId);
  EntityPtr tileEntity = world->getEntity(getAnchorTile(world, tilemap->getTile(layer, gridPos)));
  gridPos = tileEntity.get<TileComponent>()->pos; // Update to the actual position of the tile in case it's a multi-tile entity
  auto tile = tileEntity.get<TileComponent>();

  std::vector<std::pair<glm::ivec2, EntityId>> occupiedPositions;
  occupiedPositions.push_back({gridPos, tileEntity}); // The first tile IS ALWAYS the anchor tile.
  if(tileEntity.has<MultiTileComponent>()) {
    auto multiTile = tileEntity.get<MultiTileComponent>();
    auto prevMultiTile = tileEntity.get<PreviousMultiTileComponent>();
    for(const auto& pos : multiTile->occupiedPositions) {
      if(pos == multiTile->anchorPos)
        continue; // Skip the anchor tile since we already added it
      occupiedPositions.push_back({pos, world->getEntity(tilemap->getTile(layer, pos))});
    }
    multiTile->occupiedPositions = prevMultiTile->mt.occupiedPositions; // Restore previous state in case we need to undo/rollback
    multiTile->anchorPos = prevMultiTile->mt.anchorPos;
  }

  for (const auto& [pos, entity] : occupiedPositions) {
    auto tileComp = world->getEntity(entity).get<TileComponent>();
    if (b2Shape_IsValid(tileComp->shapeId)) {
      b2DestroyShape(tileComp->shapeId, false);
      tileComp->shapeId = b2_nullShapeId;
    }

    if(pos != gridPos)
      world->destroy(entity);
    tilemap->removeTile(layer, pos);
  }

  b2Body_ApplyMassFromShapes(*tilemapEntity.get<BodyComponent>());

  // Check if the tilemap has split
  checkAndHandleBreakage(world, tilemapEntityId);

  if(autoDestroyEntity) {
    world->destroy(tileEntity);
    return world->getEntity(NullEntityId);
  }

  return tileEntity;
}

bool TilemapManager::isAnchorTile(IWorldPtr world, EntityId tileId) {
  EntityPtr tileEntity = world->getEntity(tileId);
  EntityPtr parentEntity = world->getEntity(tileEntity.get<TileComponent>()->parent);
  return parentEntity.has<MultiTileComponent>();
}

EntityPtr TilemapManager::getAnchorTile(IWorldPtr world, EntityId tileId) {
  EntityPtr tileEntity = world->getEntity(tileId);
  auto tile = tileEntity.get<TileComponent>();
  EntityPtr parentEntity = world->getEntity(tile->parent);

  if(!parentEntity.has<MultiTileComponent>())
    return tileEntity;

  auto parentTile = parentEntity.get<TileComponent>();
  EntityPtr tilemapEntity = world->getEntity(parentTile->parent);
  auto tilemap = tilemapEntity.get<TilemapComponent>();
  auto multiTile = parentEntity.get<MultiTileComponent>();

  return world->getEntity(tilemap->getTile(parentTile->layer, multiTile->anchorPos));
}

Vec2 TilemapManager::floodFillTiles(RefT<TilemapComponent> tilemap, glm::ivec2 startPos, 
                                     Set<glm::ivec2>& visited) {
  std::queue<glm::ivec2> queue;
  glm::vec2 centroid = Vec2(0);
  queue.push(startPos);
  visited.insert(startPos);

  while (!queue.empty()) {
    glm::ivec2 current = queue.front();
    queue.pop();

    centroid += glm::vec2(getLocalTileCenter(current));

    // Check all 4 adjacent tiles (across all layers)
    for (int i = 0; i < 4; ++i) {
      glm::ivec2 neighbor = current + tileDirectionPos[i];
      if (tilemap->containsTileAtAnyLayer(neighbor) && visited.find(neighbor) == visited.end()) {
        visited.insert(neighbor);
        queue.push(neighbor);
      }
    }
  }

  return centroid / static_cast<float>(visited.size()); // Return the centroid of the island relative to its anchor tile
}

struct Island {
  Set<glm::ivec2> tiles;
  Vec2 localBreakoffPos;
  Vec2 worldBreakoffPos;
};

std::vector<EntityPtr> TilemapManager::checkAndHandleBreakage(IWorldPtr world, EntityId tilemapId) {
  EntityPtr tilemapEntity = world->getEntity(tilemapId);
  auto tilemap = tilemapEntity.get<TilemapComponent>();
  auto bodyComp = tilemapEntity.get<BodyComponent>();

  if (tilemap->empty())
    return {};

  // Collect all unique 2D positions across all layers
  Set<glm::ivec2> allPositions;
  for (size_t l = 0; l < TilemapComponent::LayerCount; ++l) {
    for (const auto& [pos, entity] : tilemap->getLayer(static_cast<WorldLayer>(l))) {
      allPositions.insert(pos);
    }
  }

  // Start flood fill from the first position
  glm::ivec2 startPos = *allPositions.begin();
  Set<glm::ivec2> visitedTiles;
  floodFillTiles(tilemap, startPos, visitedTiles);

  // Check if all positions were visited
  if (visitedTiles.size() == allPositions.size())
    return {}; // No split detected

  // Split detected - find all unvisited connected components
  std::vector<Island> islands;
  Set<glm::ivec2> allProcessed = visitedTiles;

  for (const auto& pos : allPositions) {
    if (allProcessed.find(pos) == allProcessed.end()) {
      Island& newIsland = islands.emplace_back();
      newIsland.localBreakoffPos = floodFillTiles(tilemap, pos, newIsland.tiles);
      newIsland.worldBreakoffPos = b2Body_GetWorldPoint(bodyComp->id, Vec2(newIsland.localBreakoffPos));
      allProcessed.insert(newIsland.tiles.begin(), newIsland.tiles.end());
    }
  }

  b2WorldId physicsWorld = world->getContext<b2WorldId>();

  // For each disconnected component, create a new tilemap
  std::vector<EntityPtr> newTilemaps;
  for (const auto& island : islands) {
    // Create new tilemap entity
    EntityPtr newTilemapEntity = world->create();
    newTilemaps.push_back(newTilemapEntity);
    newTilemapEntity.add<TilemapComponent>();
    newTilemapEntity.add<BodyComponent>();
    newTilemapEntity.add<TransformComponent>();
    auto newTilemap = newTilemapEntity.get<TilemapComponent>();
    auto newBodyComp = newTilemapEntity.get<BodyComponent>();
    auto newTransform = newTilemapEntity.get<TransformComponent>();

    // Amount of shift caused by changing the body anchor from the original tilemap's anchor to the island's centroid,
    // used to keep the new tilemap visually stable after splitting (so tiles don't appear to jump to new positions due to the change in body anchor)]
    Rot bodyRot = b2Body_GetRotation(bodyComp->id);
    Vec2 localIslandShift = bodyRot.rotate(island.localBreakoffPos / tileSize - glm::floor(island.localBreakoffPos / tileSize)) * 0.5f;

    // Calculate velocity for the fragment based on parent's motion
    Vec2 parentCenter = b2Body_GetWorldCenterOfMass(bodyComp->id);
    Vec2 fragmentCenter = island.worldBreakoffPos;
    Vec2 r = fragmentCenter - parentCenter; // Vector from parent center to fragment
    
    float parentAngularVel = b2Body_GetAngularVelocity(bodyComp->id);
    Vec2 velocityFromRotation = Vec2(-parentAngularVel * r.y, parentAngularVel * r.x);
    
    // Create physics body for new tilemap
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2Body_GetType(bodyComp->id);
    bodyDef.rotation = bodyRot;
    bodyDef.position = Vec2(island.worldBreakoffPos - localIslandShift); // Position the new body at the centroid of the island for stability
    bodyDef.linearVelocity = b2Body_GetLinearVelocity(bodyComp->id) + velocityFromRotation;
    bodyDef.angularVelocity = parentAngularVel; // Set initial angular velocity before creating body
    newBodyComp->id = b2CreateBody(physicsWorld, &bodyDef);
    newTransform->pos = bodyDef.position;
    newTransform->rot = bodyDef.rotation;

    // Move tiles to new tilemap (across all layers at island positions)
    glm::ivec2 islandAnchor = getNearestTile(island.localBreakoffPos); 
    for (const auto& oldPos : island.tiles) {
      glm::ivec2 newPos = oldPos - islandAnchor;

      for (size_t l = 0; l < TilemapComponent::LayerCount; ++l) {
        WorldLayer layer = static_cast<WorldLayer>(l);
        if (!tilemap->containsTile(layer, oldPos))
          continue;

        EntityPtr tileEntity = world->getEntity(tilemap->getTile(layer, oldPos));
        auto tile = tileEntity.get<TileComponent>();

        // Destroy old shape
        if (b2Shape_IsValid(tile->shapeId))
          b2DestroyShape(tile->shapeId, false);

        // Create new shape on new tilemap body (position relative to body center)
        Vec2 tilePos = getLocalTileCenter(newPos);
        b2Polygon tilePolygon = b2MakeOffsetBox(tileSize.x * 0.5f, tileSize.y * 0.5f, tilePos, Rot(0.0f));
        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.userData = entityToB2Data(tileEntity.id());
        shapeDef.material = tile->material;
        shapeDef.updateBodyMass = false;
        shapeDef.isSensor = !tile->collidable;
        tile->shapeId = b2CreatePolygonShape(newBodyComp->id, &shapeDef, &tilePolygon);

        // Update tile parent reference
        tile->pos = newPos;
        tile->parent = newTilemapEntity.id();

        // Move tile to new tilemap
        newTilemap->setTile(layer, newPos, tilemap->getTile(layer, oldPos));
        tilemap->removeTile(layer, oldPos);
      }
    }

    // Recalculate mass for new tilemap
    b2Body_ApplyMassFromShapes(newBodyComp->id);
  }

  // Recalculate mass for original tilemap
  b2Body_ApplyMassFromShapes(bodyComp->id);

  return newTilemaps; // Split occurred
}

void TilemapManager::enableTile(IWorldPtr world, EntityId tilemapEntityId, glm::ivec2 gridPos, bool updateMass) {
  auto tilemapEntity = world->getEntity(tilemapEntityId);
  auto tilemap = tilemapEntity.get<TilemapComponent>();
  auto bodyComp = tilemapEntity.get<BodyComponent>();
  
  std::vector<glm::ivec2> occupiedPositions;
  for(size_t l = 0; l < TilemapComponent::LayerCount; ++l) {
    WorldLayer layer = static_cast<WorldLayer>(l);
    if(!tilemap->containsTile(layer, gridPos))
      continue;
    auto tileEntity = getAnchorTile(world, tilemap->getTile(layer, gridPos));
    if(tileEntity.has<IWorld::Enabled>())
      continue;
    
    occupiedPositions.clear();
    if(tileEntity.has<MultiTileComponent>()) {
      auto multiTile = tileEntity.get<MultiTileComponent>();
      for(const auto& pos : multiTile->occupiedPositions)
        occupiedPositions.push_back(pos);
    } else
      occupiedPositions.push_back(gridPos);
      
    auto parentTile = tileEntity.get<TileComponent>();
    for(const auto& pos : occupiedPositions) {
      auto tileEntitySubTile = world->getEntity(tilemap->getTile(layer, pos));
      auto subTileComp = tileEntitySubTile.get<TileComponent>();
        
      tileEntitySubTile.enable();

      Vec2 tilePos = getLocalTileCenter(pos);
      b2Polygon tilePolygon =
          b2MakeOffsetBox(tileSize.x * 0.5f, tileSize.y * 0.5f, tilePos, Rot(0.0f));
      b2ShapeDef shapeDef = b2DefaultShapeDef();
      shapeDef.userData = entityToB2Data(tileEntitySubTile.id());
      shapeDef.material =  parentTile->material;
      shapeDef.updateBodyMass = false;
      shapeDef.isSensor = !parentTile->collidable;
      subTileComp->shapeId = b2CreatePolygonShape(bodyComp->id, &shapeDef, &tilePolygon);
    }
  }

  if(updateMass)
    b2Body_ApplyMassFromShapes(*bodyComp);
}

void TilemapManager::disableTile(IWorldPtr world, EntityId tilemapEntityId, glm::ivec2 gridPos, bool updateMass) {
  auto tilemapEntity = world->getEntity(tilemapEntityId);
  auto tilemap = tilemapEntity.get<TilemapComponent>();
  auto bodyComp = tilemapEntity.get<BodyComponent>();
  
  std::vector<glm::ivec2> occupiedPositions;
  for(size_t l = 0; l < TilemapComponent::LayerCount; ++l) {
    WorldLayer layer = static_cast<WorldLayer>(l);
    if(!tilemap->containsTile(layer, gridPos))
      continue;
    auto tileEntity = getAnchorTile(world, tilemap->getTile(layer, gridPos));
    if(!tileEntity.has<IWorld::Enabled>())
      continue;
    
    occupiedPositions.clear();
    if(tileEntity.has<MultiTileComponent>()) {
      auto multiTile = tileEntity.get<MultiTileComponent>();
      for(const auto& pos : multiTile->occupiedPositions)
        occupiedPositions.push_back(pos);
    } else
      occupiedPositions.push_back(gridPos);
    
    auto parentTile = tileEntity.get<TileComponent>();
    for(const auto& pos : occupiedPositions) {
      auto tileEntitySubTile = world->getEntity(tilemap->getTile(layer, pos));
      auto subTileComp = tileEntitySubTile.get<TileComponent>();
        
      tileEntitySubTile.disable();
      b2DestroyShape(subTileComp->shapeId, false);
      subTileComp->shapeId = b2_nullShapeId; 
    }
  }

  if(updateMass)
    b2Body_ApplyMassFromShapes(*bodyComp);
}

void TilemapManager::disableTilemap(IWorldPtr world, EntityId tilemapEntityId) {
  auto tilemapEntity = world->getEntity(tilemapEntityId);
  auto tilemap = tilemapEntity.get<TilemapComponent>();
  auto bodyComp = tilemapEntity.get<BodyComponent>();

  for(size_t l = 0; l < TilemapComponent::LayerCount; ++l) {
    WorldLayer layer = static_cast<WorldLayer>(l);
    for (const auto& [pos, entity] : tilemap->getLayer(layer)) {
      world->getEntity(entity).disable();
    }
  }

  b2Body_Disable(bodyComp->id);
}

void TilemapManager::enableTilemap(IWorldPtr world, EntityId tilemapEntityId) {
  auto tilemapEntity = world->getEntity(tilemapEntityId);
  auto tilemap = tilemapEntity.get<TilemapComponent>();
  auto bodyComp = tilemapEntity.get<BodyComponent>();

  for(size_t l = 0; l < TilemapComponent::LayerCount; ++l) {
    WorldLayer layer = static_cast<WorldLayer>(l);
    for (const auto& [pos, entity] : tilemap->getLayer(layer)) {
      world->getEntity(entity).enable();
    }
  }

  b2Body_Enable(bodyComp->id);
}

void enableRangeOfTiles(IWorldPtr world, EntityId tilemapEntityId, glm::ivec2 startPos, glm::ivec2 endPos) {
  auto tilemapEntity = world->getEntity(tilemapEntityId);
  auto tilemap = tilemapEntity.get<TilemapComponent>();
  auto bodyComp = tilemapEntity.get<BodyComponent>();
  auto& tmMgr = world->getContext<TilemapManager>();

  for(int x = startPos.x; x < endPos.x; ++x) {
    for(int y = startPos.y; y < endPos.y; ++y) {
      glm::ivec2 gridPos(x, y);
      tmMgr.enableTile(world, tilemapEntityId, gridPos, false);
    }
  }

  b2Body_ApplyMassFromShapes(*bodyComp);
}

void disableRangeOfTiles(IWorldPtr world, EntityId tilemapEntityId, glm::ivec2 startPos, glm::ivec2 endPos) {
  auto tilemapEntity = world->getEntity(tilemapEntityId);
  auto tilemap = tilemapEntity.get<TilemapComponent>();
  auto bodyComp = tilemapEntity.get<BodyComponent>();
  auto& tmMgr = world->getContext<TilemapManager>();

  for(int x = startPos.x; x < endPos.x; ++x) {
    for(int y = startPos.y; y < endPos.y; ++y) {
      glm::ivec2 gridPos(x, y);
      tmMgr.disableTile(world, tilemapEntityId, gridPos, false);
    }
  }

  b2Body_ApplyMassFromShapes(*bodyComp);
}

void enableChunk(IWorldPtr world, EntityId tilemapEntityId, glm::ivec2 chunkPos, size_t chunkSize) {
  enableRangeOfTiles(world, tilemapEntityId, chunkPos * static_cast<int>(chunkSize), chunkPos * static_cast<int>(chunkSize) + glm::ivec2(static_cast<int>(chunkSize)));
}

void disableChunk(IWorldPtr world, EntityId tilemapEntityId, glm::ivec2 chunkPos, size_t chunkSize) {
  disableRangeOfTiles(world, tilemapEntityId, chunkPos * static_cast<int>(chunkSize), chunkPos * static_cast<int>(chunkSize) + glm::ivec2(static_cast<int>(chunkSize)));
}

int updateChunkedTilemaps(IWorldPtr world, float dt) {
  auto& tmMgr = world->getContext<TilemapManager>();

  auto it = world->getWith<ChunkedTilemapComponent, TilemapComponent>();
  world->beginDefer(); 
  while(it->hasNext()) {
    EntityPtr entity = it->next();
    auto chunkedTilemap = entity.get<ChunkedTilemapComponent>();
    auto chunkedTrans = entity.get<TransformComponent>();

    Set<glm::ivec2> chunksToLoad;
    auto loaderIt = world->getWith<ChunkLoaderTag, TransformComponent>();
    while(loaderIt->hasNext()) {
      EntityPtr loaderEntity = loaderIt->next();
      // 1) Determine which chunk the loader is currently in
      auto loaderPos = loaderEntity.get<TransformComponent>()->pos;
      glm::ivec2 loaderChunk = getNearestChunk(loaderPos, chunkedTilemap->chunkSize);
    
      int minX = loaderChunk.x - (int)chunkedTilemap->loadRadius;
      int maxX = loaderChunk.x + (int)chunkedTilemap->loadRadius;
      int minY = loaderChunk.y - (int)chunkedTilemap->loadRadius;
      int maxY = loaderChunk.y + (int)chunkedTilemap->loadRadius;
    
      // 2) Determine the chunk coordinates that should be loaded.
      for(int x = minX; x <= maxX; ++x)
        for(int y = minY; y <= maxY; ++y)
            chunksToLoad.insert(glm::ivec2(x, y));
    }

    loaderIt = world->getWith<PlayerComponent, TransformComponent>();
    while(loaderIt->hasNext()) {
      EntityPtr loaderEntity = loaderIt->next();
      // 1) Determine which chunk the player is currently in
      auto loaderPos = loaderEntity.get<TransformComponent>()->pos;
      glm::ivec2 loaderChunk = getNearestChunk(loaderPos, chunkedTilemap->chunkSize);
    
      int minX = loaderChunk.x - (int)chunkedTilemap->loadRadius;
      int maxX = loaderChunk.x + (int)chunkedTilemap->loadRadius;
      int minY = loaderChunk.y - (int)chunkedTilemap->loadRadius;
      int maxY = loaderChunk.y + (int)chunkedTilemap->loadRadius;
    
      // 2) Determine the chunk coordinates that should be loaded.
      for(int x = minX; x <= maxX; ++x)
        for(int y = minY; y <= maxY; ++y)
            chunksToLoad.insert(glm::ivec2(x, y));
    }

    // 3) Disable chunks that are outside the load radius.
    std::vector<glm::ivec2> chunksToUnload;
    for(const auto& pos : chunkedTilemap->loadedChunks)
      if(!chunksToLoad.contains(pos))
        chunksToUnload.push_back(pos);
    for(const auto& chunkPos : chunksToUnload) {
      disableChunk(world, entity, chunkPos, chunkedTilemap->chunkSize);
      chunkedTilemap->loadedChunks.erase(chunkPos);
    }

    // 4) Find chunks that need to be loaded and enable them (or create them if they don't exist)
    for(const auto& chunkPos : chunksToLoad) {
      if(chunkedTilemap->loadedChunks.contains(chunkPos))
        continue; // Already loaded

      if(chunkedTilemap->chunks.contains(chunkPos)) {
        enableChunk(world, entity, chunkPos, chunkedTilemap->chunkSize);
      } else {
        // Generate tiles for the chunk using the provided generator function
        if (chunkedTilemap->generator) {
          GeneratorChunkInfo info;
          info.seed = chunkedTilemap->seed;
          info.minTilePos = chunkPos * static_cast<int>(chunkedTilemap->chunkSize);
          info.maxTilePos = chunkPos * static_cast<int>(chunkedTilemap->chunkSize) + glm::ivec2(static_cast<int>(chunkedTilemap->chunkSize));
          info.tilemapEntity = entity;
          chunkedTilemap->generator(world, info);
        }

        chunkedTilemap->chunks.insert(chunkPos);
      }

      chunkedTilemap->loadedChunks.insert(chunkPos);
    }
  }
  world->endDefer();

  return 0;
}

struct IsEntityChunkLoaderContext {
  IWorldPtr world;
  EntityId self = NullEntityId;
  EntityId hitChunkLoader = NullEntityId;
};

float isEntityChunkLoader(b2ShapeId shapeId, b2Vec2 point, b2Vec2 normal, float fraction, void* context) {
  IsEntityChunkLoaderContext& ctx = *static_cast<IsEntityChunkLoaderContext*>(context);

  EntityPtr entity = b2DataToEntity(ctx.world, b2Shape_GetUserData(shapeId));
  if(entity.isNull())
    return -1.0f; // Not an entity, ignore and continue the raycast
  if(entity == ctx.self)
    return -1.0f;
  if(entity.has<ChunkLoaderTag>()) {
    ctx.hitChunkLoader = entity;
    return 0.0f; // Hit the chunk loader, report a hit with fraction 0 to stop the raycast
  }
  return -1.0f; // Not a chunk loader, ignore and continue the raycast
}

// int updateChunkLoaders(IWorldPtr world, float dt) {
//   b2WorldId physicsWorld = world->getContext<b2WorldId>();

//   auto it = world->getWith<ChunkLoaderTag, BodyComponent, TransformComponent>();
//   world->beginDefer();
//   while(it->hasNext()) {
//     EntityPtr e = it->next();
//     auto trans = e.get<TransformComponent>();
//     if(!e.has<ChunkLoaderTag>())
//       continue;

//     IsEntityChunkLoaderContext context;
//     context.world = world;
//     context.self = e;
//     b2QueryFilter filter;
//     filter.categoryBits = PhysicsCategories::All;
//     filter.maskBits = ~PhysicsCategories::Static;
//     b2ShapeProxy proxy;
//     proxy.count = 1;
//     proxy.points[0] = trans->pos;
//     proxy.radius = 5.0f;
//     b2World_CastShape(physicsWorld, &proxy, Vec2(0), filter, isEntityChunkLoader, &context);
//     if(context.hitChunkLoader == NullEntityId)
//       continue;

//     EntityPtr chunkLoaderEntity = world->getEntity(context.hitChunkLoader);
//     chunkLoaderEntity.remove<ChunkLoaderTag>();
//   }
//   world->endDefer();

//   return 0;
// }

void loadTilemapSystems(IHost& host) {
  Pipeline& pipeline = host.getPipeline();
  IWorldPtr world = host.getWorld();

  world->component<PreviousMultiTileComponent>(false);

  // pipeline.getGroup<GameGroup>().attachToStage<GameGroup::PreTickStage>(updateChunkLoaders);
  pipeline.getGroup<GamePerSecondGroup>().attachToStage<GamePerSecondGroup::TickStage>(updateChunkedTilemaps);

  world->addContext<TilemapManager>();
  world->observe<ComponentRemovedEvent>(world->component<TileComponent>(), {}, [=](EntityPtr tile) {
    if(tile.has<TileComponent>()) {
      auto tileComp = tile.get<TileComponent>();
      if(tileComp->parent != 0 && world->isAlive(tileComp->parent)) {
        EntityPtr tilemapEntity = world->getEntity(tileComp->parent);
        if(tilemapEntity.has<TilemapComponent>()) {
          TilemapManager& manager = world->getContext<TilemapManager>();
          manager.removeTile(world, tileComp->parent, tileComp->layer, tileComp->pos, false);
        }
      }
    }
  });
}

RAMPAGE_END
