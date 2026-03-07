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

void PortComponent::serialize(capnp::MessageBuilder& builder, Ref component) {
  auto portBuilder = builder.initRoot<Schema::PortComponent>();
  auto self = component.cast<PortComponent>();

  portBuilder.setImportingInventory(self->importingInventory);
  portBuilder.setDistribution(static_cast<Schema::PortDistribution>(self->distribution));
  portBuilder.setExportingIndex((u32)self->exportingIndex);
  portBuilder.setTicksPerUpdate(self->ticksPerUpdate);
  portBuilder.setTickCounter(self->tickCounter);

  auto filterBuilder = portBuilder.initFilter((u32)self->filter.size());
  u32 i = 0;
  for (const auto& id : self->filter) {
    filterBuilder.set(i++, id);
  }

  portBuilder.setIsOn(self->isOn);
  portBuilder.setExportingConveyor(self->exportingConveyor);
}

void PortComponent::deserialize(capnp::MessageReader& reader, const IdMapper& idMapper, Ref component) {
  auto portReader = reader.getRoot<Schema::PortComponent>();
  auto self = component.cast<PortComponent>();

  self->importingInventory = idMapper.resolve(portReader.getImportingInventory());
  self->distribution = static_cast<PortDistribution>((u8)portReader.getDistribution());
  self->exportingIndex = portReader.getExportingIndex();
  self->ticksPerUpdate = portReader.getTicksPerUpdate();
  self->tickCounter = portReader.getTickCounter();

  self->filter.clear();
  for (const auto& id : portReader.getFilter()) {
    self->filter.insert(idMapper.resolve(id));
  }

  self->isOn = portReader.getIsOn();
  self->exportingConveyor = idMapper.resolve(portReader.getExportingConveyor());
}

void PortComponent::fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& jsonValue) {
  auto self = component.cast<PortComponent>();
  auto compJson = jsonValue.as<JSchema::PortComponent>();

  if (compJson->hasImportingInventory())
    self->importingInventory = loader.getAsset(compJson->getImportingInventory());

  if (compJson->hasDistribution()) {
    auto dist = compJson->getDistribution();
    if (dist == "RoundRobin") self->distribution = PortDistribution::RoundRobin;
    else if (dist == "Priority") self->distribution = PortDistribution::Priority;
    else if (dist == "First") self->distribution = PortDistribution::First;
  }

  if (compJson->hasTicksPerUpdate())
    self->ticksPerUpdate = compJson->getTicksPerUpdate();

  if (compJson->hasFilter()) {
    for (const auto& item : compJson->getFilter()) {
      self->filter.insert(loader.getAsset(item));
    }
  }

  if (compJson->hasIsOn())
    self->isOn = compJson->getIsOn();

  if (compJson->hasExportingConveyor())
    self->exportingConveyor = loader.getAsset(compJson->getExportingConveyor());
}

void ConveyorComponent::serialize(capnp::MessageBuilder& builder, Ref component) {
  auto conveyorBuilder = builder.initRoot<Schema::ConveyorComponent>();
  auto self = component.cast<ConveyorComponent>();

  auto itemsBuilder = conveyorBuilder.initItemsInTransit((u32)self->itemsInTransit.size());
  for (u32 i = 0; i < self->itemsInTransit.size(); ++i) {
    auto& item = self->itemsInTransit[i];
    itemsBuilder[i].setCount(item.stack.count);
    itemsBuilder[i].setItemId(item.stack.itemId);
    itemsBuilder[i].setInvIndex(item.invIndex);
    itemsBuilder[i].setCurVirtualDistance(item.curVirtualDistance);
  }

  conveyorBuilder.setMaxItemsInTransit((u32)self->maxItemsInTransit);

  auto invBuilder = conveyorBuilder.initInventories((u32)self->inventories.size());
  for (u32 i = 0; i < self->inventories.size(); ++i) {
    auto& inv = self->inventories[i];
    invBuilder[i].setInventoryId(inv.inventoryId);
    invBuilder[i].setVirtualDistance(inv.virtualDistance);
  }
}

void ConveyorComponent::deserialize(capnp::MessageReader& reader, const IdMapper& idMapper, Ref component) {
  auto conveyorReader = reader.getRoot<Schema::ConveyorComponent>();
  auto self = component.cast<ConveyorComponent>();

  self->itemsInTransit.clear();
  for (const auto& itemReader : conveyorReader.getItemsInTransit()) {
    ConveyorComponent::ItemInTransit item;
    item.stack.count = itemReader.getCount();
    item.stack.itemId = idMapper.resolve(itemReader.getItemId());
    item.invIndex = itemReader.getInvIndex();
    item.curVirtualDistance = itemReader.getCurVirtualDistance();
    self->itemsInTransit.push_back(item);
  }

  self->maxItemsInTransit = conveyorReader.getMaxItemsInTransit();

  self->inventories.clear();
  for (const auto& invReader : conveyorReader.getInventories()) {
    ConveyorComponent::InventoryDistance inv;
    inv.inventoryId = idMapper.resolve(invReader.getInventoryId());
    inv.virtualDistance = invReader.getVirtualDistance();
    self->inventories.push_back(inv);
  }
}

void ConveyorPartComponent::serialize(capnp::MessageBuilder& builder, Ref component) {
  auto partBuilder = builder.initRoot<Schema::ConveyorPartComponent>();
  auto self = component.cast<ConveyorPartComponent>();

  partBuilder.setConveyorId(self->conveyorId);

  auto dirsBuilder = partBuilder.initInputDirections((u32)self->inputDirections.size());
  for (u32 i = 0; i < self->inputDirections.size(); ++i) {
    dirsBuilder.set(i, static_cast<u8>(self->inputDirections[i]));
  }

  partBuilder.setVirtualDistanceIncrease(self->virtualDistanceIncrease);
}

void ConveyorPartComponent::deserialize(capnp::MessageReader& reader, const IdMapper& idMapper, Ref component) {
  auto partReader = reader.getRoot<Schema::ConveyorPartComponent>();
  auto self = component.cast<ConveyorPartComponent>();

  self->conveyorId = idMapper.resolve(partReader.getConveyorId());

  self->inputDirections.clear();
  for (const auto& dir : partReader.getInputDirections()) {
    self->inputDirections.push_back(static_cast<TileDirection>(dir));
  }

  self->virtualDistanceIncrease = partReader.getVirtualDistanceIncrease();
}

void ConveyorPartComponent::fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& jsonValue) {
  auto self = component.cast<ConveyorPartComponent>();
  auto compJson = jsonValue.as<JSchema::ConveyorPartComponent>();

  if (compJson->hasConveyorId())
    self->conveyorId = loader.getAsset(compJson->getConveyorId());

  if (compJson->hasInputDirections()) {
    for (const auto& dir : compJson->getInputDirections()) {
      if (dir == "Up") self->inputDirections.push_back(TileDirection::Up);
      else if (dir == "Right") self->inputDirections.push_back(TileDirection::Right);
      else if (dir == "Down") self->inputDirections.push_back(TileDirection::Down);
      else if (dir == "Left") self->inputDirections.push_back(TileDirection::Left);
    }
  }

  if (compJson->hasVirtualDistanceIncrease())
    self->virtualDistanceIncrease = compJson->getVirtualDistanceIncrease();
}

RAMPAGE_END
