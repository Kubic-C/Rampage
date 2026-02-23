#pragma once

#include "../../common/common.hpp"

RAMPAGE_START

using AssetId = u32;
using InventoryId = u32;

struct InventoryComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component) {
    auto builderRoot = builder.initRoot<Schema::InventoryData>();
    auto inventory = component.cast<InventoryComponent>();

    // builderRoot.set(inventory->id);
  }

  static void deserialize(capnp::MessageReader& reader, Ref component) {
    auto readerRoot = reader.getRoot<Schema::InventoryData>();
    auto inventory = component.cast<InventoryComponent>();

    // inventory->id = readerRoot.getId();
  }

  InventoryId id;
};

struct ItemAttrStackCost {
  static void serialize(capnp::MessageBuilder& builder, Ref component) {
    auto builderRoot = builder.initRoot<Schema::ItemAttrStackCost>();
    auto attr = component.cast<ItemAttrStackCost>();

    builderRoot.setStackCost(attr->stackCost);
  }

  static void deserialize(capnp::MessageReader& reader, Ref component) {
    auto readerRoot = reader.getRoot<Schema::ItemAttrStackCost>();
    auto attr = component.cast<ItemAttrStackCost>();

    attr->stackCost = readerRoot.getStackCost();
  }

  u8 stackCost = 1;
};

struct ItemAttrIcon {
  static void serialize(capnp::MessageBuilder& builder, Ref component) {
    auto iconBuilder = builder.initRoot<Schema::ItemAttrIcon>();
    auto icon = component.cast<ItemAttrIcon>();

    iconBuilder.setName(icon->icon.getId().toStdString()); 
  }

  static void deserialize(capnp::MessageReader& reader, Ref component) {
    auto iconReader = reader.getRoot<Schema::ItemAttrIcon>();
    auto icon = component.cast<ItemAttrIcon>();

    icon->icon = tgui::Texture(iconReader.getName().cStr()); // Load texture by name
  }

  tgui::Texture icon; 
};

struct ItemAttrUnique : SerializableTag {
  ItemAttrUnique() = default;

  ItemAttrUnique(glz::make_reflectable) {}
};

struct ItemAttrTile {
  static void serialize(capnp::MessageBuilder& builder, Ref component) {
    auto tileBuilder = builder.initRoot<Schema::ItemAttrTileComponent>();
    auto tile = component.cast<ItemAttrTile>();
    tileBuilder.setAssetId(tile->tileId);
  }

  static void deserialize(capnp::MessageReader& reader, Ref component) {
    auto tileReader = reader.getRoot<Schema::ItemAttrTileComponent>();
    auto tile = component.cast<ItemAttrTile>();
    tile->tileId = tileReader.getAssetId();
  }

  AssetId tileId;
};

struct TileItemComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component) {
    auto tileItemBuilder = builder.initRoot<Schema::TileItemComponent>();
    auto tileItem = component.cast<TileItemComponent>();

    tileItemBuilder.setItem(tileItem->item);
  }

  static void deserialize(capnp::MessageReader& reader, Ref component) {
    auto tileItemReader = reader.getRoot<Schema::TileItemComponent>();
    auto tileItem = component.cast<TileItemComponent>();
    
    tileItem->item = tileItemReader.getItem();
  }

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
