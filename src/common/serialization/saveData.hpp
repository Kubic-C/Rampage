#pragma once

#include "../commondef.hpp"
#include "../ecs/id.hpp"
#include "../ecs/world->hpp"
#include "idRemapper.hpp"

RAMPAGE_START

/**
 * Serialized representation of a single item stack in inventory.
 */
struct SerializedItemStack {
  u32 maxStackCost = 64;
  i32 stackCount = 0;
  EntityId item = 0;  // Entity ID (will be remapped during load)
  
  // Note: UI elements (tgui::BitmapButton) are NOT serialized
  // They are reconstructed during inventory UI initialization
};

/**
 * Serialized representation of an inventory.
 *
 * Preserves the inventory structure and item references, but NOT UI state.
 * InventoryId is preserved to maintain cross-references.
 */
struct SerializedInventory {
  InventoryId id;
  std::string name;
  u16 rows = 3;
  u16 cols = 3;
  Map<glm::u16vec2, SerializedItemStack> items;
};

/**
 * Serialized representation of entity data.
 *
 * Stores components attached to an entity. Exact format depends on your
 * component serialization system (already exists in your codebase).
 *
 * Note: This is a placeholder. Your actual structure may differ based on
 * how components implement SerializableComponent<T>.
 */
struct SerializedEntity {
  EntityId id;
  std::vector<u8> componentData;  // Opaque component data from world
  // In practice, you'd store (ComponentId, componentData) pairs
};

/**
 * Complete savegame data ready for serialization.
 *
 * This is the intermediate representation between disk and IWorldPtr.
 * It contains all original IDs from the save file before remapping.
 */
struct SaveData {
  std::string gameVersion;  // For compatibility checking
  std::string timestamp;    // When this was saved

  // World data
  std::vector<SerializedEntity> entities;
  std::vector<SerializedInventory> inventories;

  /**
   * Apply remapping to all IDs in this SaveData.
   *
   * This must be called after loading from disk but BEFORE applying to world->
   * It updates all EntityId and InventoryId references to use the new allocations.
   *
   * @param entityRemapper Maps EntityId (old → new)
   * @param inventoryRemapper Maps InventoryId (old → new)
   */
  void applyRemapping(const IdRemapper<EntityId>& entityRemapper,
                      const IdRemapper<InventoryId>& inventoryRemapper) {
    // Update entity IDs
    for (auto& entity : entities) {
      if (entityRemapper.has(entity.id)) {
        entity.id = entityRemapper.get(entity.id);
      }
    }

    // Update inventory IDs and item references within them
    for (auto& inventory : inventories) {
      if (inventoryRemapper.has(inventory.id)) {
        inventory.id = inventoryRemapper.get(inventory.id);
      }

      // Update item entity references in inventory slots
      for (auto& [pos, stack] : inventory.items) {
        if (stack.item != 0 && entityRemapper.has(stack.item)) {
          stack.item = entityRemapper.get(stack.item);
        }
      }
    }
  }
};

RAMPAGE_END
