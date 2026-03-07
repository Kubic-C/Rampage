#include "inventory.hpp"
#include "../../core/module.hpp"
#include "../components/body.hpp"
#include "../components/sprite.hpp"
#include "../../render/render.hpp"
#include "../components/shapes.hpp"
#include "tilemap.hpp"

RAMPAGE_START

RefT<InventoryComponent> InventoryManager::createInventory(IWorldPtr world, EntityId entityId, u16 cols, u16 rows) {
  EntityPtr entity = world->getEntity(entityId);
  entity.add<InventoryComponent>();
  auto invComp = entity.get<InventoryComponent>();

  invComp->cols = cols;
  invComp->rows = rows;
  invComp->items.resize(cols * rows); // Initialize slots

  return invComp;
}

void InventoryManager::dropItem(IWorldPtr world, EntityId itemId, Vec2 dropPosition, u32 count) {
  auto texMap = world->getFirstWith(world->set<TextureMapInUseTag>()).get<TextureMap3DComponent>();

  EntityPtr itemEntity = world->getEntity(itemId); 
  auto itemComp = itemEntity.get<ItemComponent>();
  
  if(itemComp->isUnique && itemEntity.has<AssetTag>()) {
    itemEntity = world->clone(itemEntity);
    itemEntity.remove<AssetTag>();
    itemComp = itemEntity.get<ItemComponent>();
  }

  // For unique items, spawn one entity per item (count represents number of unique instances)
  // For stackable items, spawn entities based on stack size limits
  u32 itemsPerStack = 1;
  if (!itemComp->isUnique) {
    itemsPerStack = itemComp->maxStackSize / itemComp->stackCost;
    if (itemsPerStack == 0) itemsPerStack = 1; // At least 1 item per stack
  }

  // Spawn multiple entities if needed
  u32 remaining = count;
  float offsetAngle = 0.0f;
  while (remaining > 0) {
    u32 countInThisStack = std::min(remaining, itemsPerStack);
    remaining -= countInThisStack;

    EntityPtr droppedItem = world->create();
    droppedItem.add<TransformComponent>();
    droppedItem.add<BodyComponent>();
    droppedItem.add<SpriteComponent>();
    droppedItem.add<ItemStackComponent>();
    droppedItem.add<CircleRenderComponent>();
    droppedItem.add<ItemDroppedTag>();
    
    // Offset slightly for multiple spawned items
    Vec2 offsetPos = dropPosition;
    if (remaining > 0 || count > itemsPerStack) {
      offsetPos.x += 0.3f * std::cos(offsetAngle);
      offsetPos.y += 0.3f * std::sin(offsetAngle);
      offsetAngle += 1.5708f; // 45 degree increments
    }

    auto transComp = droppedItem.get<TransformComponent>();
    transComp->pos = offsetPos;
    
    constexpr float radius = 0.125f;

    auto bodyComp = droppedItem.get<BodyComponent>();
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = {offsetPos.x, offsetPos.y};
    bodyDef.userData = entityToB2Data(droppedItem);
    bodyDef.linearDamping = 1;
    bodyDef.angularDamping = 0.5;
    bodyComp->id = b2CreateBody(world->getContext<b2WorldId>(), &bodyDef);
    b2Circle circle{0};
    circle.radius = radius;
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.filter.categoryBits = Item | Friendly;
    shapeDef.filter.maskBits = Friendly | Static;
    shapeDef.userData = entityToB2Data(droppedItem);
    b2CreateCircleShape(bodyComp->id, &shapeDef, &circle);  
    b2Body_EnableContactEvents(bodyComp->id, true);

    auto spriteComp = droppedItem.get<SpriteComponent>();
    spriteComp->scaling = 0.5f;
    u32 texIndex = texMap->getSprite(getFilename(std::string(itemComp->icon.getId())));
    SpriteComponent::SubSprite subSprite;
    subSprite.addLayer(texIndex, Vec2(0), 0, WorldLayer::Item);
    spriteComp->subSprites.push_back({subSprite});

    auto circleRenderComp = droppedItem.get<CircleRenderComponent>();
    float stackFullness = std::min(1.0f, (float)itemComp->getTotalStackCost(countInThisStack) / itemComp->maxStackSize);
    circleRenderComp->color = glm::vec3(1.0f, 1.0f - stackFullness, 0.0f); // More red as stack gets fuller
    circleRenderComp->radius = 1.5f * radius + radius * stackFullness; // Larger radius for fuller stacks
    circleRenderComp->z = WorldLayerItemZ;

    // For unique items, clone the item entity for each drop
    EntityId itemIdForStack = itemEntity;
    if (itemComp->isUnique && countInThisStack > 0) {
      // Clone the item entity
      EntityPtr clonedItem = world->clone(itemEntity);
      itemIdForStack = clonedItem;
    }

    auto itemStackComp = droppedItem.get<ItemStackComponent>();
    itemStackComp->count = countInThisStack;
    itemStackComp->itemId = itemIdForStack;
  }
}


void InventoryManager::dropItem(IWorldPtr world, EntityId invEntityId, u16 x, u16 y, Vec2 dropPosition, u32 count) {
  EntityPtr invEntity = world->getEntity(invEntityId);
  if (!invEntity.has<InventoryComponent>()) {
    return; // Invalid inventory entity
  }

  auto invComp = invEntity.get<InventoryComponent>();
  if (x >= invComp->cols || y >= invComp->rows) {
    return; // Invalid slot position
  }

  ItemStackComponent slot = invComp->getSlot(x, y); // copy not reference
  if (slot.itemId == 0 || slot.count == 0) {
    return; // Slot is already empty
  }

  EntityPtr itemEntity = world->getEntity(slot.itemId);
  if (!itemEntity.has<ItemComponent>()) {
    return; // Invalid item entity in slot
  }

  u32 amountDropped = removeItem(world, invEntityId, x, y, count);
  if(amountDropped < count)
    count = amountDropped;
  if(amountDropped == 0)
    return;

  dropItem(world, slot.itemId, dropPosition, count);
}

bool canPlace(EntityPtr itemEntity, Vec2 placePosition) {
  auto world = itemEntity.world();

  if (!itemEntity.has<ItemComponent>() || !itemEntity.has<ItemPlaceableComponent>()) {
    return false; // Invalid item entity in slot
  }

  return !getTileAtPos(world, placePosition).isNull();
}

bool InventoryManager::placeItem(IWorldPtr world, EntityId itemId, Vec2 placePosition, u32 pickableCount) {
  auto tmMgr = world->getContext<TilemapManager>();

  EntityPtr itemEntity = world->getEntity(itemId);
  if (!canPlace(itemEntity, placePosition)) {
    return false; // Invalid item entity or placement position
  }

  EntityId entityToPlaceId;
  EntityPtr entityToClone = world->getEntity(itemEntity.get<ItemPlaceableComponent>()->entityId);
  if(entityToClone.has<TileComponent>()) {
    EntityPtr existingTileEntity = getTileAtPos(world, placePosition);
    WorldLayer layer = getNextLayer(existingTileEntity.get<TileComponent>()->layer);
    EntityPtr tilemapEntity = world->getEntity(existingTileEntity.get<TileComponent>()->parent);
    if(!tmMgr.canInsert(world, tilemapEntity.id(), layer, existingTileEntity.get<TileComponent>()->pos, entityToClone)) {
      return false; // Can't place on this tile
    }
  
    entityToPlaceId = world->clone(entityToClone).id();
    tmMgr.insertTile(world, tilemapEntity.id(), layer, existingTileEntity.get<TileComponent>()->pos, entityToPlaceId);
  } else {
    entityToPlaceId = world->clone(entityToClone).id();
    EntityPtr entityToPlace = world->getEntity(entityToPlaceId);
    entityToPlace.add<TransformComponent>();
    auto transComp = entityToPlace.get<TransformComponent>();
    transComp->pos = placePosition;
  }

  EntityPtr entityToPlace = world->getEntity(entityToPlaceId);
  entityToPlace.remove<AssetTag>(); // Asset tag is usually in placeable componetns
  entityToPlace.add<ItemPlacedTag>();
  entityToPlace.add<ItemStackComponent>();
  auto stackComp = entityToPlace.get<ItemStackComponent>();
  stackComp->itemId = itemId;
  stackComp->count = pickableCount;

  std::cout << "Placed entity " << entityToPlaceId << " at position (" << placePosition.x << ", " << placePosition.y << ")\n";

  return true;
}

void InventoryManager::placeItem(IWorldPtr world, EntityId invEntityId, u16 x, u16 y, Vec2 placePosition) {
  EntityPtr invEntity = world->getEntity(invEntityId);
  if (!invEntity.has<InventoryComponent>()) {
    return; // Invalid inventory entity
  }

  auto invComp = invEntity.get<InventoryComponent>();
  if (x >= invComp->cols || y >= invComp->rows) {
    return; // Invalid slot position
  }

  ItemStackComponent slot = invComp->getSlot(x, y); // copy not reference
  if (slot.itemId == 0 || slot.count == 0) {
    return; // Slot is already empty
  }

  if(placeItem(world, slot.itemId, placePosition)) {
    removeItem(world, invEntityId, x, y);
  }
}

u32 InventoryManager::addItem(IWorldPtr world, EntityId invEntityId, EntityId itemEntityId__, u32 count) {
  EntityPtr itemEntity = world->getEntity(itemEntityId__);
  EntityPtr invEntity = world->getEntity(invEntityId);

  // Failure Conditions
  // 1) Invalid item entity
  // 2) No free slots and item is not stackable or doesn't match existing stacks
  // 3) count is 0 (nothing to add)

  if (!itemEntity.has<ItemComponent>() || 
      !invEntity.has<InventoryComponent>() || 
      count == 0) {
    return 0; // Invalid item entity
  }

  auto itemComp = itemEntity.get<ItemComponent>();
  auto invComp = invEntity.get<InventoryComponent>();

  if(itemComp->isUnique && itemEntity.has<AssetTag>()) {
    itemEntity = world->clone(itemEntity);
    itemEntity.remove<AssetTag>();
    itemComp = itemEntity.get<ItemComponent>();
  }

  u32 maxStackSize = itemComp->maxStackSize / itemComp->stackCost;
  if(!itemComp->isUnique) {
    // Find slots containing the same item type to stack onto
    for(ItemStackComponent& slot : invComp->items) {
      if (slot.itemId == itemEntity) {
        u32 toAdd = std::min(maxStackSize - slot.count, count);
        slot.count += toAdd;
        count -= toAdd;

        if (count == 0)
          return 0;
      }
    }
  } else {
    maxStackSize = 1;
    count = 1; 
  }

  // Find empty slots to place remaining items
  for (ItemStackComponent& slot : invComp->items) {
    if (slot.itemId == 0) {
      u32 toAdd = std::min(maxStackSize, count);
      slot.itemId = itemEntity;
      slot.count = toAdd;
      count -= toAdd;

      if (count == 0)
        return 0;
    }
  }

  return count;
}

u32 InventoryManager::removeItem(IWorldPtr world, EntityId invEntityId, u16 x, u16 y, u32 count) {
  EntityPtr invEntity = world->getEntity(invEntityId);
  if (!invEntity.has<InventoryComponent>()) {
    return 0; // Invalid inventory entity
  }

  auto invComp = invEntity.get<InventoryComponent>();
  if (x >= invComp->cols || y >= invComp->rows) {
    return 0; // Invalid slot position
  }

  ItemStackComponent& slot = invComp->getSlot(x, y);
  if (slot.itemId == 0 || slot.count == 0) {
    return 0; // Slot is already empty
  }

  EntityPtr itemEntity = world->getEntity(slot.itemId);
  if (!itemEntity.has<ItemComponent>()) {
    return 0; // Invalid item entity in slot
  }

  return removeFromSlot(slot, count);
}

u32 InventoryManager::removeItemByType(IWorldPtr world, EntityId invEntityId, EntityId itemEntity, u32 count) {
  EntityPtr invEntity = world->getEntity(invEntityId);
  if (!invEntity.has<InventoryComponent>()) {
    return 0; // Invalid inventory entity
  }

  auto invComp = invEntity.get<InventoryComponent>();
  u32 totalRemoved = 0;

  for(ItemStackComponent& slot : invComp->items) {
    if (slot.itemId == itemEntity) {
      u32 removed = removeFromSlot(slot, count);
      totalRemoved += removed;
      count -= removed;
      if (count == 0)
        return totalRemoved; // Removed requested amount
    }
  }

  return totalRemoved;
}

u32 InventoryManager::removeFromSlot(ItemStackComponent& slot, u32 count) {
  u32 toRemove = std::min(slot.count, count);
  slot.count -= toRemove;
  if (slot.count == 0) {
    slot.itemId = 0; // Clear item reference if stack is empty
  }
  return toRemove;
}

bool InventoryManager::moveItems(IWorldPtr world, EntityId srcInvEntityId, u16 srcX, u16 srcY,
                                  EntityId dstInvEntityId, u16 dstX, u16 dstY,
                                  u32 count) {
  EntityPtr srcInvEntity = world->getEntity(srcInvEntityId);
  EntityPtr dstInvEntity = world->getEntity(dstInvEntityId);

  if (!srcInvEntity.has<InventoryComponent>() || !dstInvEntity.has<InventoryComponent>()) {
    return false;
  }

  auto srcInv = srcInvEntity.get<InventoryComponent>();
  auto dstInv = dstInvEntity.get<InventoryComponent>();

  if (srcX >= srcInv->cols || srcY >= srcInv->rows || 
      dstX >= dstInv->cols || dstY >= dstInv->rows) {
    return false; // Invalid slot positions
  }

  ItemStackComponent& srcSlot = srcInv->getSlot(srcX, srcY);
  ItemStackComponent& dstSlot = dstInv->getSlot(dstX, dstY);

  if (srcSlot.itemId == 0) {
    return false; // Source slot is empty
  }

  // If count is 0, move all from source
  if (count == 0) {
    count = srcSlot.count;
  }

  EntityPtr itemEntity = world->getEntity(srcSlot.itemId);
  if (!itemEntity.has<ItemComponent>()) {
    return false;
  }

  auto itemComp = itemEntity.get<ItemComponent>();

  // If destination is empty or same item type, try to merge
  if (dstSlot.itemId == 0 || dstSlot.itemId == srcSlot.itemId) {
    if (!itemComp->isUnique && mergeStacks(world, srcInvEntityId, srcX, srcY, dstInvEntityId, dstX, dstY)) {
      return true;
    }
  }

  // Fall back to swap if merge didn't work
  return swapSlots(world, srcInvEntityId, srcX, srcY, dstInvEntityId, dstX, dstY);
}

bool InventoryManager::swapSlots(IWorldPtr world, EntityId invEntityIdA, u16 x1, u16 y1,
                                 EntityId invEntityIdB, u16 x2, u16 y2) {
  EntityPtr invEntityA = world->getEntity(invEntityIdA);
  EntityPtr invEntityB = world->getEntity(invEntityIdB);

  if (!invEntityA.has<InventoryComponent>() || !invEntityB.has<InventoryComponent>()) {
    return false;
  }

  auto invA = invEntityA.get<InventoryComponent>();
  auto invB = invEntityB.get<InventoryComponent>();

  if (x1 >= invA->cols || y1 >= invA->rows || 
      x2 >= invB->cols || y2 >= invB->rows) {
    return false;
  }

  ItemStackComponent& slotA = invA->getSlot(x1, y1);
  ItemStackComponent& slotB = invB->getSlot(x2, y2);

  // Simple swap
  std::swap(slotA, slotB);
  return true;
}

bool InventoryManager::mergeStacks(IWorldPtr world, EntityId invEntityIdA, u16 x1, u16 y1,
                                   EntityId invEntityIdB, u16 x2, u16 y2) {
  EntityPtr invEntityA = world->getEntity(invEntityIdA);
  EntityPtr invEntityB = world->getEntity(invEntityIdB);

  if (!invEntityA.has<InventoryComponent>() || !invEntityB.has<InventoryComponent>()) {
    return false;
  }

  auto invA = invEntityA.get<InventoryComponent>();
  auto invB = invEntityB.get<InventoryComponent>();

  if (x1 >= invA->cols || y1 >= invA->rows || 
      x2 >= invB->cols || y2 >= invB->rows) {
    return false;
  }

  ItemStackComponent& slotA = invA->getSlot(x1, y1);
  ItemStackComponent& slotB = invB->getSlot(x2, y2);

  // Can't merge if either slot is empty or items don't match
  if (slotA.itemId == 0 || slotB.itemId == 0 || slotA.itemId != slotB.itemId) {
    return false;
  }

  EntityPtr itemEntity = world->getEntity(slotA.itemId);
  if (!itemEntity.has<ItemComponent>()) {
    return false;
  }

  auto itemComp = itemEntity.get<ItemComponent>();

  // Can't merge unique items
  if (itemComp->isUnique) {
    return false;
  }

  u32 maxStackSize = itemComp->maxStackSize;
  u32 spaceInB = maxStackSize - slotB.count;

  if (spaceInB == 0) {
    return false; // Destination stack is full
  }

  u32 toMove = std::min(spaceInB, slotA.count);
  slotA.count -= toMove;
  slotB.count += toMove;

  if (slotA.count == 0) {
    slotA.itemId = 0;
  }

  return true;
}

glm::u16vec2 InventoryManager::findItemStack(IWorldPtr world, EntityId invEntityId, EntityId itemEntity) {
  EntityPtr invEnt = world->getEntity(invEntityId);
  if (!invEnt.has<InventoryComponent>()) {
    return {USHRT_MAX, USHRT_MAX};
  }

  auto inv = invEnt.get<InventoryComponent>();

  for (u16 y = 0; y < inv->rows; ++y) {
    for (u16 x = 0; x < inv->cols; ++x) {
      if (inv->getSlot(x, y).itemId == itemEntity) {
        return {x, y};
      }
    }
  }

  return {USHRT_MAX, USHRT_MAX};
}

glm::u16vec2 InventoryManager::findEmptySlot(IWorldPtr world, EntityId invEntityId) {
  EntityPtr invEnt = world->getEntity(invEntityId);
  if (!invEnt.has<InventoryComponent>()) {
    return {USHRT_MAX, USHRT_MAX};
  }

  auto inv = invEnt.get<InventoryComponent>();

  for (u16 y = 0; y < inv->rows; ++y) {
    for (u16 x = 0; x < inv->cols; ++x) {
      if (inv->getSlot(x, y).itemId == 0) {
        return {x, y};
      }
    }
  }

  return {USHRT_MAX, USHRT_MAX};
}

bool InventoryManager::canFitItem(IWorldPtr world, EntityId invEntityId, EntityId itemEntity, u32 count) {
  EntityPtr invEnt = world->getEntity(invEntityId);
  EntityPtr itemEnt = world->getEntity(itemEntity);

  if (!invEnt.has<InventoryComponent>() || !itemEnt.has<ItemComponent>()) {
    return false;
  }

  auto inv = invEnt.get<InventoryComponent>();
  auto itemComp = itemEnt.get<ItemComponent>();

  if (itemComp->isUnique) {
    // Each unique item needs its own slot, count should be 1
    u32 emptySlots = getUsedSlotCount(world, invEntityId);
    return (inv->rows * inv->cols - emptySlots) >= 1;
  }

  // For stackables, check if we can fit the items
  u32 remainingToAdd = count;
  u32 maxStackSize = itemComp->maxStackSize;

  // Try existing stacks
  for (const auto& slot : inv->items) {
    if (slot.itemId == itemEntity) {
      u32 spaceInStack = maxStackSize - slot.count;
      u32 toAdd = std::min(spaceInStack, remainingToAdd);
      remainingToAdd -= toAdd;
      if (remainingToAdd == 0) return true;
    }
  }

  // Count empty slots
  u32 emptySlots = 0;
  for (const auto& slot : inv->items) {
    if (slot.itemId == 0) emptySlots++;
  }

  // How many full stacks do we need?
  u32 slotsNeeded = (remainingToAdd + maxStackSize - 1) / maxStackSize;
  return slotsNeeded <= emptySlots;
}

u32 InventoryManager::countItemType(IWorldPtr world, EntityId invEntityId, EntityId itemEntity) {
  EntityPtr invEnt = world->getEntity(invEntityId);
  if (!invEnt.has<InventoryComponent>()) {
    return 0;
  }

  auto inv = invEnt.get<InventoryComponent>();
  u32 total = 0;

  for (const auto& slot : inv->items) {
    if (slot.itemId == itemEntity) {
      total += slot.count;
    }
  }

  return total;
}

bool InventoryManager::isInventoryFull(IWorldPtr world, EntityId invEntityId) const {
  EntityPtr invEnt = world->getEntity(invEntityId);
  if (!invEnt.has<InventoryComponent>()) {
    return false;
  }

  auto inv = invEnt.get<InventoryComponent>();
  for (const auto& slot : inv->items) {
    if (slot.itemId == 0) {
      return false;
    }
  }
  return true;
}

u32 InventoryManager::getUsedSlotCount(IWorldPtr world, EntityId invEntityId) const {
  EntityPtr invEnt = world->getEntity(invEntityId);
  if (!invEnt.has<InventoryComponent>()) {
    return 0;
  }

  auto inv = invEnt.get<InventoryComponent>();
  u32 used = 0;

  for (const auto& slot : inv->items) {
    if (slot.itemId != 0) {
      used++;
    }
  }

  return used;
}

u32 InventoryManager::calculateSlotsNeeded(IWorldPtr world, EntityId itemEntity, u32 count) const {
  EntityPtr itemEnt = world->getEntity(itemEntity);
  if (!itemEnt.has<ItemComponent>()) {
    return 0;
  }

  auto itemComp = itemEnt.get<ItemComponent>();

  if (itemComp->isUnique) {
    return 1; // Unique items always take 1 slot
  }

  // Calculate slots needed for stackable items
  u32 maxStackSize = itemComp->maxStackSize;
  return (count + maxStackSize - 1) / maxStackSize;
}

int updateInventoryViews(IWorldPtr world, float dt) {
  auto it = world->getWith({world->component<InventoryViewComponent>()});
  while(it->hasNext()) {
    EntityPtr entity = it->next();
    auto viewComp = entity.get<InventoryViewComponent>();
    viewComp->update(entity, world->getContext<tgui::Gui>());
  }

  return 0;
}

void loadInventorySystems(IHost& host) {
  auto world = host.getWorld();

  host.getPipeline().getGroup<GameGroup>().attachToStage<GameGroup::PreTickStage>(updateInventoryViews);
}

RAMPAGE_END