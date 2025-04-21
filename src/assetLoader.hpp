#pragma once

#include "item.hpp"
#include "tilePrefabs.hpp"

typedef u32 AssetId;
typedef u32 SpriteId;

class AssetLoader {
  typedef EntityId ItemId;
  
  using Asset = std::variant<SpriteId, ItemId, TilePrefabId>;
public:
  
  template<typename T>
  AssetId createAsset() {
    return m_idMgr.generate();
  }

private:
  IdManager<AssetId> m_idMgr;
  Map<AssetId, Asset> m_assets;
};