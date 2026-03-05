#pragma once

#include "../components/inventory.hpp"

RAMPAGE_START

/**
 * InventoryManager - Core ECS-based inventory operations
 * Works directly with world entities containing InventoryComponent
 * All item references are EntityIds pointing to items with ItemComponent
 */
class InventoryManager {
public:
  // ========================================================================
  // INITIALIZATION
  // ========================================================================

  RefT<InventoryComponent> createInventory(IWorldPtr world, EntityId entityId, u16 cols = 8, u16 rows = 5);

  // ========================================================================
  // ITEM EVENTS
  // ========================================================================

  void dropItem(IWorldPtr world, EntityId itemId, Vec2 dropPosition, u32 count = 1);

  void dropItem(IWorldPtr world, EntityId invEntityId, u16 x, u16 y, Vec2 dropPosition, u32 count = 1);

  // ========================================================================
  // ITEM ADDITION/REMOVAL
  // ========================================================================

  /**
   * Try to add items to an inventory
   * For stackable items: fills existing stacks first, then uses empty slots
   * For unique items: uses one slot per item
   * @return the amount of items not added. If 0, all items were added successfully.
   */
  u32 addItem(IWorldPtr world, EntityId invEntityId, EntityId itemEntity, u32 count = 1);

  /**
   * Remove items from a specific slot
   * @param count How many to remove (for stackables)
   * @return Number of items actually removed
   */
  u32 removeItem(IWorldPtr world, EntityId invEntityId, u16 x, u16 y, u32 count = 1);

  /**
   * Find and remove first matching item type
   * @return Number of items removed
   */
  u32 removeItemByType(IWorldPtr world, EntityId invEntityId, EntityId itemEntity, u32 count = 1);

  // ========================================================================
  // ITEM MOVEMENT / REORGANIZATION
  // ========================================================================

  /**
   * Move items from one slot to another (possibly different inventory)
   * Intelligently merges stacks or swaps based on item types
   * @param count How many to move (0 = all from source)
   * @return true if move succeeded
   */
  bool moveItems(IWorldPtr world, EntityId srcInvEntityId, u16 srcX, u16 srcY,
                 EntityId dstInvEntityId, u16 dstX, u16 dstY,
                 u32 count = 0);

  /**
   * Swap entire slot contents between two positions
   */
  bool swapSlots(IWorldPtr world, EntityId invEntityIdA, u16 x1, u16 y1,
                 EntityId invEntityIdB, u16 x2, u16 y2);

  /**
   * Merge stacks of the same item type
   * Only works for stackable items (not isUnique)
   * @return true if merge succeeded
   */
  bool mergeStacks(IWorldPtr world, EntityId invEntityIdA, u16 x1, u16 y1,
                   EntityId invEntityIdB, u16 x2, u16 y2);

  // ========================================================================
  // QUERIES / SEARCH
  // ========================================================================

  /**
   * Find the first slot containing the given item type
   * @return {x, y} position or {USHRT_MAX, USHRT_MAX} if not found
   */
  glm::u16vec2 findItemStack(IWorldPtr world, EntityId invEntityId, EntityId itemEntity);

  /**
   * Find first empty slot
   * @return {x, y} position or {USHRT_MAX, USHRT_MAX} if none
   */
  glm::u16vec2 findEmptySlot(IWorldPtr world, EntityId invEntityId);

  /**
   * Check if inventory has space for more of this item
   */
  bool canFitItem(IWorldPtr world, EntityId invEntityId, EntityId itemEntity, u32 count = 1);

  /**
   * Count total instances of an item type across all slots
   */
  u32 countItemType(IWorldPtr world, EntityId invEntityId, EntityId itemEntity);

  // ========================================================================
  // INVENTORY STATE
  // ========================================================================

  bool isInventoryFull(IWorldPtr world, EntityId invEntityId) const;

  u32 getUsedSlotCount(IWorldPtr world, EntityId invEntityId) const;

  /**
   * Calculate how many slots an item count would require
   */
  u32 calculateSlotsNeeded(IWorldPtr world, EntityId itemEntity, u32 count) const;

private:
  // Helper to remove items from a slot and return how many were removed
  u32 removeFromSlot(ItemStackComponent& slot, u32 count);
};

void loadInventorySystems(IHost& host);

RAMPAGE_END