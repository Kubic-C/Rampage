#include "item.hpp"

RAMPAGE_START

// InventoryComponent
void InventoryComponent::serialize(capnp::MessageBuilder& builder, Ref component) {
  auto builderRoot = builder.initRoot<Schema::InventoryData>();
  auto inventory = component.cast<InventoryComponent>();

  // builderRoot.set(inventory->id);
}

void InventoryComponent::deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component) {
  auto readerRoot = reader.getRoot<Schema::InventoryData>();
  auto inventory = component.cast<InventoryComponent>();

  // inventory->id = readerRoot.getId();
}

void InventoryComponent::fromJson(Ref component, AssetLoader loader, const json& compJson) {
  auto self = component.cast<InventoryComponent>();
}

// ItemAttrStackCost
void ItemAttrStackCost::serialize(capnp::MessageBuilder& builder, Ref component) {
  auto builderRoot = builder.initRoot<Schema::ItemAttrStackCost>();
  auto attr = component.cast<ItemAttrStackCost>();

  builderRoot.setStackCost(attr->stackCost);
}

void ItemAttrStackCost::deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component) {
  auto readerRoot = reader.getRoot<Schema::ItemAttrStackCost>();
  auto attr = component.cast<ItemAttrStackCost>();

  attr->stackCost = readerRoot.getStackCost();
}

void ItemAttrStackCost::fromJson(Ref component, AssetLoader loader, const json& compJson) {
  auto self = component.cast<ItemAttrStackCost>();
  if (compJson.contains("stackCost") && compJson["stackCost"].is_number_unsigned()) {
    self->stackCost = compJson["stackCost"];
  }
}

// ItemAttrIcon
void ItemAttrIcon::serialize(capnp::MessageBuilder& builder, Ref component) {
  auto iconBuilder = builder.initRoot<Schema::ItemAttrIcon>();
  auto icon = component.cast<ItemAttrIcon>();
}

void ItemAttrIcon::deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component) {
  auto iconReader = reader.getRoot<Schema::ItemAttrIcon>();
  auto icon = component.cast<ItemAttrIcon>();

  icon->icon = tgui::Texture(iconReader.getName().cStr()); // Load texture by name
}

void ItemAttrIcon::fromJson(Ref component, AssetLoader loader, const json& compJson) {
  auto self = component.cast<ItemAttrIcon>();
  if (compJson.contains("iconPath") && compJson["iconPath"].is_string()) {
    std::string iconPath = compJson["iconPath"];
    self->icon = tgui::Texture(iconPath);
  }
}

// ItemAttrTile
void ItemAttrTile::serialize(capnp::MessageBuilder& builder, Ref component) {
  auto tileBuilder = builder.initRoot<Schema::ItemAttrTileComponent>();
  auto tile = component.cast<ItemAttrTile>();
  tileBuilder.setAssetId(tile->tileId.value());
}

void ItemAttrTile::fromJson(Ref component, AssetLoader loader, const json& compJson) {
  auto self = component.cast<ItemAttrTile>();
}

void ItemAttrTile::deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component) {
  auto tileReader = reader.getRoot<Schema::ItemAttrTileComponent>();
  auto tile = component.cast<ItemAttrTile>();
  tile->tileId = AssetId(tileReader.getAssetId());
}

void TileItemComponent::fromJson(Ref component, AssetLoader loader, const json& compJson) {
  auto self = component.cast<TileItemComponent>();
  if (compJson.contains("item") && compJson["item"].is_string()) {
    std::string itemName = compJson["item"];
    self->item = loader.getAsset(itemName);
  }
}

// TileItemComponent
void TileItemComponent::serialize(capnp::MessageBuilder& builder, Ref component) {
  auto tileItemBuilder = builder.initRoot<Schema::TileItemComponent>();
  auto tileItem = component.cast<TileItemComponent>();

  tileItemBuilder.setItem(tileItem->item);
}

void TileItemComponent::deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component) {
  auto tileItemReader = reader.getRoot<Schema::TileItemComponent>();
  auto tileItem = component.cast<TileItemComponent>();
  
  tileItem->item = tileItemReader.getItem();
}

RAMPAGE_END
