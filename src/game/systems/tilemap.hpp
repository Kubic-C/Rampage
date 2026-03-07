#pragma once

#include "../../common/common.hpp"
#include "../components/tilemap.hpp"

RAMPAGE_START

class TilemapManager {
public:
  // Check if a tile can be inserted into a tilemap at grid position on the specified layer
  bool canInsert(IWorldPtr world, EntityId tilemapId, WorldLayer layer, glm::ivec2 gridPos, EntityId tileEntity);

  // Insert a tile entity into a tilemap at grid position on the specified layer
  bool insertTile(IWorldPtr world, EntityId tilemapId, WorldLayer layer, glm::ivec2 gridPos, EntityId tileEntity);
  
  // Remove a tile and break its joints
  EntityPtr removeTile(IWorldPtr world, EntityId tilemapId, WorldLayer layer, glm::ivec2 gridPos, bool autoDestroyEntity);

  // Is the tile an anchor tile?
  bool isAnchorTile(IWorldPtr world, EntityId tileId);

  // Get the anchor tile, Useful for multi-tile entities, also works on single-tile entities
  EntityPtr getAnchorTile(IWorldPtr world, EntityId tileId);

  // Detects if a tilemap has split into multiple disconnected components and creates new tilemaps
  std::vector<EntityPtr> checkAndHandleBreakage(IWorldPtr world, EntityId tilemapId);

private:
  // Flood fill helper to find all connected tiles starting from a position (2D across all layers)
  Vec2 floodFillTiles(RefT<TilemapComponent> tilemap, glm::ivec2 startPos, 
                      Set<glm::ivec2>& visited);
};

void loadTilemapSystems(IHost& host);

RAMPAGE_END
