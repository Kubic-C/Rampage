#include "tilemap.hpp"
#include "../module.hpp"

RAMPAGE_START

template<typename T>
struct PreviousComponent : T {};

bool TilemapManager::insertTile(IWorldPtr world, EntityId tilemapEntityId, glm::ivec3 gridPos, EntityId tileEntityId, bool collidable) {
  EntityPtr tilemapEntity = world->getEntity(tilemapEntityId);
  EntityPtr tileEntity = world->getEntity(tileEntityId);
  auto tilemap = tilemapEntity.get<TilemapComponent>();
  auto bodyComp = tilemapEntity.get<BodyComponent>();
  if(!tileEntity.has<TileComponent>()) 
    return false;
  auto tile = tileEntity.get<TileComponent>();

  // Insert single/multi tile in the tilemap

  // 1) (READONLY) Determine all occupied positions based on whether it's a multi-tile or single tile entity
  std::vector<std::pair<glm::ivec3, EntityId>> occupiedPositions;
  occupiedPositions.push_back({gridPos, tileEntityId}); // The first tile IS ALWAYS the anchor tile.
  if(tileEntity.has<MultiTileComponent>()) {
    // Read from multi-tile component to determine all occupied positions,
    // and add them to the tilemap as well with the same parent
    auto multiTile = tileEntity.get<MultiTileComponent>();
    for(const auto& pos : multiTile->occupiedPositions) {
      if(pos == multiTile->anchorPos)
        continue; // Skip the anchor tile since we already added it
      occupiedPositions.push_back({pos + gridPos, NullEntityId});
    }
  }

  // 2) (READONLY) Failure cases; 
  //  - If there's an existing tile, operation fails
  //  - If tile already belongs to another tilemap, operation fails
  //  - If tile has existing joints, operation fails
  for(auto& pos : occupiedPositions)
    if(tilemap->tiles.contains(pos.first) || pos.first.z > TilemapComponent::maxTopLayer || pos.first.z < TilemapComponent::minTopLayer)
      return false;
  if(tile->parent != 0)
    return false;
  if(b2Shape_IsValid(tile->shapeId))
    return false;

  // 3) Update tilemap and tile data and create additional subtiles as needed.
  tile->pos = gridPos;
  tile->parent = tilemapEntityId;
  tilemap->tiles[gridPos] = tileEntityId;
  for(auto&[pos, entity] : occupiedPositions) {
    if(pos == gridPos)
      continue; // Skip the anchor tile since we already added it
    EntityPtr subTile = world->create();
    subTile.add<TileComponent>();
    auto subTileComp = subTile.get<TileComponent>();
    subTileComp->parent = tileEntityId;
    subTileComp->material = tile->material;
    subTileComp->pos = pos;

    entity = subTile;
    tilemap->tiles[pos] = subTile;
  }
  if(tileEntity.has<MultiTileComponent>()) {
    auto multiTile = tileEntity.get<MultiTileComponent>();
    tileEntity.add<PreviousComponent<MultiTileComponent>>();
    auto prevMultiTile = tileEntity.get<PreviousComponent<MultiTileComponent>>();
    prevMultiTile->anchorPos = multiTile->anchorPos; // Store previous state for undo/rollback purposes
    prevMultiTile->occupiedPositions = multiTile->occupiedPositions;
    // Write back to multitile the updated positions
    multiTile->anchorPos = gridPos;
    multiTile->occupiedPositions.clear();
    for (const auto& pos : occupiedPositions) { 
      multiTile->occupiedPositions.push_back(pos.first);
    }
  }
    
  // 4) Create Box2D shapes for each tile and subtile
  if(collidable) {
    for(const auto& [pos, entity] : occupiedPositions) {
      auto tileComp = world->getEntity(entity).get<TileComponent>();

      Vec2 tilePos = TilemapComponent::getLocalTileCenter(glm::ivec2(pos));
      b2Polygon tilePolygon =
          b2MakeOffsetBox(tileSize.x * 0.5f, tileSize.y * 0.5f, tilePos, Rot(0.0f));
      b2ShapeDef shapeDef = b2DefaultShapeDef();
      shapeDef.userData = entityToB2Data(tileEntity.id());
      shapeDef.material = tile->material;
      shapeDef.updateBodyMass = false;
      tileComp->shapeId = b2CreatePolygonShape(bodyComp->id, &shapeDef, &tilePolygon);
    }

    b2Body_ApplyMassFromShapes(*bodyComp);
  }

  return true;
}

EntityPtr TilemapManager::removeTile(IWorldPtr world, EntityId tilemapEntityId, glm::ivec3 gridPos) {
  EntityPtr tilemapEntity = world->getEntity(tilemapEntityId);
  auto tilemap = tilemapEntity.get<TilemapComponent>();

  if(!tilemap->tiles.contains(gridPos))
    return world->getEntity(NullEntityId);
  EntityPtr tileEntity = world->getEntity(getAnchorTile(world, tilemap->tiles[gridPos]));
  gridPos = tileEntity.get<TileComponent>()->pos; // Update to the actual position of the tile in case it's a multi-tile entity
  auto tile = tileEntity.get<TileComponent>();

  std::vector<std::pair<glm::ivec3, EntityId>> occupiedPositions;
  occupiedPositions.push_back({gridPos, tileEntity}); // The first tile IS ALWAYS the anchor tile.
  if(tileEntity.has<MultiTileComponent>()) {
    auto multiTile = tileEntity.get<MultiTileComponent>();
    auto prevMultiTile = tileEntity.get<PreviousComponent<MultiTileComponent>>();
    multiTile->occupiedPositions = prevMultiTile->occupiedPositions; // Restore previous state in case we need to undo/rollback
    multiTile->anchorPos = prevMultiTile->anchorPos;
    for(const auto& pos : multiTile->occupiedPositions) {
      if(pos == multiTile->anchorPos)
        continue; // Skip the anchor tile since we already added it
      occupiedPositions.push_back({pos, world->getEntity(tilemap->tiles[pos])});
    }
  }

  for (const auto& [pos, entity] : occupiedPositions) {
    auto tileComp = world->getEntity(entity).get<TileComponent>();
    if (b2Shape_IsValid(tileComp->shapeId)) {
      b2DestroyShape(tileComp->shapeId, false);
      tileComp->shapeId = b2_nullShapeId;
    }

    if(pos != gridPos)
      world->destroy(entity);
    tilemap->tiles.erase(pos);
  }

  b2Body_ApplyMassFromShapes(*tilemapEntity.get<BodyComponent>());

  // Check if the tilemap has split
  checkAndHandleBreakage(world, tilemapEntityId);

  return world->getEntity(tileEntity);
}

bool TilemapManager::isAnchorTile(IWorldPtr world, EntityId tileId) {
  EntityPtr tileEntity = world->getEntity(tileId);
  EntityPtr parentEntity = world->getEntity(tileEntity.get<TileComponent>()->parent);
  return parentEntity.has<MultiTileComponent>();
}

EntityPtr TilemapManager::getAnchorTile(IWorldPtr world, EntityId tileId) {
  EntityPtr tileEntity = world->getEntity(tileId);
  EntityPtr parentEntity = world->getEntity(tileEntity.get<TileComponent>()->parent);
  auto tile = tileEntity.get<TileComponent>();

  if(!parentEntity.has<MultiTileComponent>())
    return tileEntity;

  EntityPtr tilemapEntity = world->getEntity(parentEntity.get<TileComponent>()->parent);
  auto tilemap = tilemapEntity.get<TilemapComponent>();
  auto multiTile = parentEntity.get<MultiTileComponent>();

  return world->getEntity(tilemap->tiles[multiTile->anchorPos]);
}

Vec2 TilemapManager::floodFillTiles(RefT<TilemapComponent> tilemap, glm::ivec3 startPos, 
                                     Set<glm::ivec3>& visited) {
  std::queue<glm::ivec3> queue;
  glm::vec2 centroid = Vec2(0);
  queue.push(startPos);
  visited.insert(startPos);

  while (!queue.empty()) {
    glm::ivec3 current = queue.front();
    queue.pop();

    centroid += glm::vec2(TilemapComponent::getLocalTileCenter(current));

    // Check all 4 adjacent tiles
    for (int i = 0; i < 4; ++i) {
      glm::ivec3 neighbor = current + tileDirectionPos[i];
      if (tilemap->tiles.contains(neighbor) && visited.find(neighbor) == visited.end()) {
        visited.insert(neighbor);
        queue.push(neighbor);
      }
    }
  }

  return centroid / static_cast<float>(visited.size()); // Return the centroid of the island relative to its anchor tile
}

struct Island {
  Set<glm::ivec3> tiles;
  Vec2 localBreakoffPos;
  Vec2 worldBreakoffPos;
};

std::vector<EntityPtr> TilemapManager::checkAndHandleBreakage(IWorldPtr world, EntityId tilemapId) {
  EntityPtr tilemapEntity = world->getEntity(tilemapId);
  auto tilemap = tilemapEntity.get<TilemapComponent>();
  auto bodyComp = tilemapEntity.get<BodyComponent>();

  if (tilemap->tiles.empty())
    return {};

  // Start flood fill from the first tile
  glm::ivec3 startPos = tilemap->tiles.begin()->first;
  Set<glm::ivec3> visitedTiles;
  floodFillTiles(tilemap, startPos, visitedTiles);

  // Check if all tiles were visited
  if (visitedTiles.size() == tilemap->tiles.size())
    return {}; // No split detected

  // Split detected - find all unvisited connected components
  std::vector<Island> islands;
  Set<glm::ivec3> allProcessed = visitedTiles;

  for (const auto& [pos, entity] : tilemap->tiles) {
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

    // Create physics body for new tilemap
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2Body_GetType(bodyComp->id);
    bodyDef.rotation = bodyRot;
    bodyDef.position = Vec2(island.worldBreakoffPos - localIslandShift); // Position the new body at the centroid of the island for stability
    bodyDef.linearVelocity = b2Body_GetLinearVelocity(bodyComp->id);
    bodyDef.angularVelocity = b2Body_GetAngularVelocity(bodyComp->id);
    newBodyComp->id = b2CreateBody(physicsWorld, &bodyDef);
    newTransform->pos = bodyDef.position;
    newTransform->rot = bodyDef.rotation;

    // Move tiles to new tilemap
    glm::ivec3 islandAnchor = TilemapComponent::getNearestTile(island.localBreakoffPos); 
    for (const auto& oldPos : island.tiles) {
      glm::ivec3 newPos = glm::ivec3(glm::ivec2(oldPos) - glm::ivec2(islandAnchor), oldPos.z);
      EntityPtr tileEntity = world->getEntity(tilemap->tiles[oldPos]);
      auto tile = tileEntity.get<TileComponent>();

      // Destroy old shape
      b2ShapeId oldShapeId = tile->shapeId;
      if (b2Shape_IsValid(oldShapeId))
        b2DestroyShape(oldShapeId, false);

      // Create new shape on new tilemap body (position relative to body center)
      Vec2 tilePos = TilemapComponent::getLocalTileCenter(glm::ivec2(newPos));
      b2Polygon tilePolygon = b2MakeOffsetBox(tileSize.x * 0.5f, tileSize.y * 0.5f, tilePos, Rot(0.0f));
      b2ShapeDef shapeDef = b2DefaultShapeDef();
      shapeDef.userData = entityToB2Data(tileEntity.id());
      shapeDef.material = tile->material;
      shapeDef.updateBodyMass = false;
      tile->shapeId = b2CreatePolygonShape(newBodyComp->id, &shapeDef, &tilePolygon);

      // Update tile parent reference
      tile->pos = newPos;
      tile->parent = newTilemapEntity.id();

      // Move tile to new tilemap
      newTilemap->tiles[newPos] = tilemap->tiles[oldPos];
    }

    // Recalculate mass for new tilemap
    b2Body_ApplyMassFromShapes(newBodyComp->id);
  }

  // Remove moved tiles from original tilemap and recalculate its mass
  for (const auto& island : islands)
    for (const auto& pos : island.tiles)
      tilemap->tiles.erase(pos);
  b2Body_ApplyMassFromShapes(bodyComp->id);

  return newTilemaps; // Split occurred
}

void loadTilemapSystems(IHost& host) {
  Pipeline& pipeline = host.getPipeline();
  IWorldPtr world = host.getWorld();

  world->component<PreviousComponent<MultiTileComponent>>(false);

  world->addContext<TilemapManager>();
}

RAMPAGE_END
