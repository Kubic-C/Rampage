#pragma once

#include "../../common/common.hpp"
#include "../components/tilemap.hpp"

RAMPAGE_START

class TilemapManager {
public:
  // Insert a tile entity into a tilemap at grid position
  bool insertTile(IWorldPtr world, EntityId tilemapId, glm::ivec3 gridPos, EntityId tileEntity, bool collidable = true);
  
  // Remove a tile and break its joints
  EntityPtr removeTile(IWorldPtr world, EntityId tilemapId, glm::ivec3 gridPos);

  // Is the tile an anchor tile?
  bool isAnchorTile(IWorldPtr world, EntityId tileId);

  // Get the anchor tile, Useful for multi-tile entities, also works on single-tile entities
  EntityPtr getAnchorTile(IWorldPtr world, EntityId tileId);

  // Detects if a tilemap has split into multiple disconnected components and creates new tilemaps
  std::vector<EntityPtr> checkAndHandleBreakage(IWorldPtr world, EntityId tilemapId);

private:
  // Flood fill helper to find all connected tiles starting from a position
  Vec2 floodFillTiles(RefT<TilemapComponent> tilemap, glm::ivec3 startPos, 
                      Set<glm::ivec3>& visited);
};

void loadTilemapSystems(IHost& host);

RAMPAGE_END
