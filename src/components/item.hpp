#pragma once

#include "../assetLoader.hpp"

using InventoryId = u32;

struct InventoryComponent {
  InventoryId id;
};

struct ItemAttrStackCost {
  u8 stackCost = 1;
};

struct ItemAttrIcon {
  tgui::Texture icon;
};

struct ItemAttrUnique {};

struct ItemAttrTile {
  AssetId tileId;
};

struct TileItemComponent {
  EntityId item;
};