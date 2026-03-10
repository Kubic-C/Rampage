#pragma once

#include "../../common/common.hpp"
#include "../components/tilemap.hpp"
#include "../components/chunkedTilemap.hpp"

RAMPAGE_START

class TilemapManager {
public:
  // Check if a tile can be inserted into a tilemap at grid position on the specified layer
  bool canInsert(IWorldPtr world, EntityId tilemapId, glm::ivec2 gridPos, TileDirection rotation, EntityId tileEntity);

  // Insert a tile entity into a tilemap at grid position on the specified layer
  bool insertTile(IWorldPtr world, EntityId tilemapId, glm::ivec2 gridPos, TileDirection rotation, EntityId tileEntity);
  
  // Remove a tile and break its joints
  EntityPtr removeTile(IWorldPtr world, EntityId tilemapId, WorldLayer layer, glm::ivec2 gridPos);

  // Is the tile an anchor tile?
  bool isAnchorTile(const Tile& tile);

  // Get the anchor tile, Useful for multi-tile entities, also works on single-tile entities
  Tile& getAnchorTile(IWorldPtr world, const Tile& tile);

  // Detects if a tilemap has split into multiple disconnected components and creates new tilemaps
  std::vector<EntityPtr> checkAndHandleBreakage(IWorldPtr world, EntityId tilemapId);

  void enableTile(IWorldPtr world, EntityId tilemap, glm::ivec2 gridPos, bool updateMass = true);
  void disableTile(IWorldPtr world, EntityId tilemap, glm::ivec2 gridPos, bool updateMass = true);

  void disableTilemap(IWorldPtr world, EntityId tilemap);
  void enableTilemap(IWorldPtr world, EntityId tilemap);

private:
  // Flood fill helper to find all connected tiles starting from a position (2D across all layers)
  Vec2 floodFillTiles(RefT<TilemapComponent> tilemap, glm::ivec2 startPos, 
                      Set<glm::ivec2>& visited);
};

void loadTilemapSystems(IHost& host);

RAMPAGE_END
