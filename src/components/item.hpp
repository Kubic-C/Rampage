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

struct ItemAttrUnique {
  ItemAttrUnique() = default;
  ItemAttrUnique(glz::make_reflectable) {}
};

struct ItemAttrTile {
  AssetId tileId;
};

struct TileItemComponent {
  EntityId item;
};

template <>
struct glz::meta<ItemAttrStackCost> {
  using T = ItemAttrStackCost;
  static constexpr auto value = object("stackCost", &T::stackCost);
};

template <>
struct glz::meta<ItemAttrUnique> {
  using T = ItemAttrUnique;
  static constexpr auto value = object();
};