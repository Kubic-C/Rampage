#pragma once

#include "../../common/common.hpp"

RAMPAGE_START

using AssetId = u32;
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

RAMPAGE_END

template <>
struct glz::meta<rmp::ItemAttrStackCost> {
  using T = rmp::ItemAttrStackCost;
  static constexpr auto value = object("stackCost", &T::stackCost);
};

template <>
struct glz::meta<rmp::ItemAttrUnique> {
  using T = rmp::ItemAttrUnique;
  static constexpr auto value = object();
};
