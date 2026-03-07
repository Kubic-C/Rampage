#include "items.hpp"

RAMPAGE_START

void ItemPlaceableComponent::serialize(capnp::MessageBuilder& builder, Ref component) {
  auto placeableBuilder = builder.initRoot<Schema::ItemPlaceableComponent>();
  auto self = component.cast<ItemPlaceableComponent>();

  placeableBuilder.setEntityId(self->entityId);
}

void ItemPlaceableComponent::deserialize(capnp::MessageReader& reader, const IdMapper& idMapper, Ref component) {
  auto placeableReader = reader.getRoot<Schema::ItemPlaceableComponent>();
  auto self = component.cast<ItemPlaceableComponent>();

  self->entityId = idMapper.resolve(placeableReader.getEntityId());
}

void ItemPlaceableComponent::fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& jsonValue) {
  auto self = component.cast<ItemPlaceableComponent>();
  auto compJson = jsonValue.as<JSchema::ItemPlaceableComponent>();

  if (compJson->hasEntityId())
    self->entityId = loader.getAsset(compJson->getEntityId());
}

void ItemComponent::serialize(capnp::MessageBuilder& builder, Ref component) {
  auto itemBuilder = builder.initRoot<Schema::ItemComponent>();
  auto self = component.cast<ItemComponent>();

  itemBuilder.setName(self->name);
  itemBuilder.setDescription(self->description);
  itemBuilder.setMaxStackSize(self->maxStackSize);
  itemBuilder.setStackCost(self->stackCost);
  itemBuilder.setIsUnique(self->isUnique);
  itemBuilder.setIconPath((std::string)self->icon.getId()); // Assuming TGUI's Texture has a method to get the original file path
}

void ItemComponent::deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component) {
  auto itemReader = reader.getRoot<Schema::ItemComponent>();
  auto self = component.cast<ItemComponent>();

  self->name = itemReader.getName();
  self->description = itemReader.getDescription();
  self->maxStackSize = itemReader.getMaxStackSize();
  self->stackCost = itemReader.getStackCost();
  self->isUnique = itemReader.getIsUnique();
  
  std::string pathStr = itemReader.getIconPath().cStr();
  if (std::filesystem::exists(pathStr)) {
    self->icon = tgui::Texture(pathStr); // Load texture from path
  }
}

void ItemComponent::fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& jsonValue) {
  auto self = component.cast<ItemComponent>();
  auto compJson = jsonValue.as<JSchema::ItemComponent>();

  if (compJson->hasName())
    self->name = compJson->getName();

  if (compJson->hasDescription())
    self->description = compJson->getDescription();

  if (compJson->hasMaxStackSize())
    self->maxStackSize = compJson->getMaxStackSize();

  if (compJson->hasStackCost())
    self->stackCost = compJson->getStackCost();

  if (compJson->hasIsUnique())
    self->isUnique = compJson->getIsUnique();

  // Load icon from asset loader if iconAsset is specified
  if (compJson->hasIconAsset()) {
    std::string pathStr = compJson->getIconAsset();
    if (std::filesystem::exists(pathStr)) {
      self->icon = tgui::Texture(pathStr); // Load texture from path
    } else {
      component.getWorld()->getHost().log("Could not load icon asset for ItemComponent: %s for %s", pathStr.c_str(), self->name.c_str());
    }
  }
}

void ItemStackComponent::serialize(capnp::MessageBuilder& builder, Ref component) {
  auto stackBuilder = builder.initRoot<Schema::ItemStackComponent>();
  auto self = component.cast<ItemStackComponent>();

  stackBuilder.setCount(self->count);
  stackBuilder.setItemId(self->itemId);
}

void ItemStackComponent::deserialize(capnp::MessageReader& reader, const IdMapper& idMapper, Ref component) {
  auto stackReader = reader.getRoot<Schema::ItemStackComponent>();
  auto self = component.cast<ItemStackComponent>();

  self->count = stackReader.getCount();
  self->itemId = idMapper.resolve(stackReader.getItemId());
}

void ItemStackComponent::fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& jsonValue) {
  auto self = component.cast<ItemStackComponent>();
  auto compJson = jsonValue.as<JSchema::ItemStackComponent>();

  if (compJson->hasCount())
    self->count = compJson->getCount();

  if (compJson->hasItemId())
    self->itemId = compJson->getItemId();
}

RAMPAGE_END
