#pragma once

/**
 * PRACTICAL EXAMPLE: Using IdRemapper in Your Game
 *
 * This shows real-world usage patterns.
 */

#include "saveCoordinator.hpp"

RAMPAGE_START

/**
 * Example 1: Basic Load on Game Startup
 */
void exampleGameStartup(IWorldPtr world, InventoryManager& invMgr) {
  SaveCoordinator coordinator(world, invMgr);

  // Try to load last save
  if (coordinator.loadGame("saves/autosave.json", true /* clear existing */)) {
    // Game state fully restored with ID conflicts resolved
    std::cout << "Game loaded successfully\n";
  } else {
    // Load failed or save doesn't exist, start new game
    std::cout << "Starting new game\n";
    initializeNewGame(world, invMgr);
  }
}

/**
 * Example 2: Persistent Saving During Play
 */
void examplePeriodicSave(IWorldPtr world, InventoryManager& invMgr) {
  static SaveCoordinator coordinator(world, invMgr);
  static auto lastSave = std::chrono::steady_clock::now();

  auto now = std::chrono::steady_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastSave);

  // Auto-save every 5 minutes
  if (elapsed.count() >= 300) {
    if (coordinator.saveGame("saves/autosave.json")) {
      std::cout << "Auto-saved at " << getTimestamp() << "\n";
    } else {
      std::cerr << "Auto-save failed\n";
    }
    lastSave = now;
  }
}

/**
 * Example 3: Manual Save
 */
void exampleManualSave(IWorldPtr world, InventoryManager& invMgr) {
  SaveCoordinator coordinator(world, invMgr);

  Map<std::string, std::string> metadata;
  metadata["player_name"] = "Sawyer";
  metadata["location"] = "Castle Floor 5";
  metadata["playtime_hours"] = "12.5";

  if (coordinator.saveGame("saves/manual_slot_1.json", metadata)) {
    std::cout << "Game saved to slot 1\n";
  } else {
    std::cerr << "Failed to save game\n";
  }
}

/**
 * Example 4: Loading Different Saves in Sequence
 *
 * This demonstrates the conflict resolution in action.
 * Each load creates fresh mappings, preventing ID collisions.
 */
void exampleLoadMultipleSaves() {
  IWorldPtr world(host);
  InventoryManager invMgr(world);
  SaveCoordinator coordinator(world, invMgr);

  // Scenario: Player loads save A, plays for a bit, then loads save B
  // Without proper conflict resolution, loaded IDs from B would collide with A's.

  // Load first save (world is empty, all IDs reused)
  coordinator.loadGame("saves/save_a.json", true /* clear */);
  std::cout << "Loaded Save A\n";

  // Play for a bit, maybe add new entities
  Entity newEntity = world->create();  // Gets ID 100 (or whatever)
  // ...

  // Switch to different save (clear and load)
  coordinator.loadGame("saves/save_b.json", true /* clear */);
  std::cout << "Loaded Save B\n";
  // Save B's entities restored with correct IDs, no conflicts despite ID reuse
}

/**
 * Example 5: Understanding ID Remapping
 *
 * Shows what happens internally when there's a conflict.
 */
void exampleUnderstandingRemapping() {
  IWorldPtr world(host);
  InventoryManager invMgr(world);

  // Manually create some entities to simulate existing state
  Entity e1 = world->create();  // Gets ID 1
  Entity e2 = world->create();  // Gets ID 2
  Entity e3 = world->create();  // Gets ID 3

  // Now we load a save that had: EntityId 1, 2, 3, 4
  // IDs 1, 2, 3 are taken in current world
  // ID 4 is free

  IdRemapper<EntityId> remapper;

  // ID 1 exists → allocate new
  EntityId loadId1 = remapper.apply(1, world->m_idMgr);  // Returns new ID (5)

  // ID 2 exists → allocate new
  EntityId loadId2 = remapper.apply(2, world->m_idMgr);  // Returns new ID (6)

  // ID 3 exists → allocate new
  EntityId loadId3 = remapper.apply(3, world->m_idMgr);  // Returns new ID (7)

  // ID 4 is free → reuse
  EntityId loadId4 = remapper.apply(4, world->m_idMgr);  // Returns 4 (reused)

  // Now remapper has mappings:
  // 1→5, 2→6, 3→7, 4→4

  // When SaveData is applied, any references to the original IDs
  // are updated to the new IDs automatically
  std::cout << "Mapping: 1->" << remapper.get(1) << ", "
            << "2->" << remapper.get(2) << ", "
            << "3->" << remapper.get(3) << ", "
            << "4->" << remapper.get(4) << "\n";
  // Output: Mapping: 1->5, 2->6, 3->7, 4->4
}

/**
 * Example 6: Checking Remap Results
 *
 * Debug/introspect remapping decisions.
 */
void exampleDebugRemapping() {
  IdRemapper<EntityId> remapper;
  IWorldPtr world(host);

  for (EntityId originalId = 1; originalId <= 10; ++originalId) {
    EntityId loadedId = remapper.apply(originalId, world->m_idMgr);

    if (originalId == loadedId) {
      std::cout << "ID " << originalId << " → reused (no conflict)\n";
    } else {
      std::cout << "ID " << originalId << " → remapped to " << loadedId
                << " (conflict detected)\n";
    }
  }

  std::cout << "\nTotal remapped: " << remapper.getRemappedCount() << " IDs\n";
}

/**
 * Example 7: Inventory-Specific Scenario
 *
 * Shows how inventory IDs and item references are handled together.
 */
void exampleInventoryRemapping() {
  // Saved data:
  // - Inventory ID 5 with 3 items
  //   - Item from Entity 10 at position [0,0]
  //   - Item from Entity 11 at position [1,0]
  //   - Item from Entity 12 at position [2,0]

  // Current world already has:
  // - Inventory ID 5 (conflict!)
  // - Entity IDs 10, 11 are taken (conflict!)
  // - Entity ID 12 is free (no conflict)

  IdRemapper<EntityId> entityRemapper;
  IdRemapper<InventoryId> invRemapper;
  IWorldPtr world(host);
  InventoryManager invMgr(world);

  // Remap inventory
  InventoryId savedInvId = 5;
  InventoryId loadedInvId = invRemapper.apply(savedInvId, invMgr.m_idMgr);
  std::cout << "Inventory: " << savedInvId << " → " << loadedInvId << "\n";

  // Remap item entity IDs
  EntityId loadedItemId10 = entityRemapper.apply(10, world->m_idMgr);
  EntityId loadedItemId11 = entityRemapper.apply(11, world->m_idMgr);
  EntityId loadedItemId12 = entityRemapper.apply(12, world->m_idMgr);

  std::cout << "Item entities: 10→" << loadedItemId10 << ", "
            << "11→" << loadedItemId11 << ", "
            << "12→" << loadedItemId12 << "\n";

  // When SaveData applies remapping:
  // - Inventory ID becomes loadedInvId
  // - Items at [0,0], [1,0], [2,0] now reference loadedItemId10, 11, 12
  // - Everything stays consistent!
}

RAMPAGE_END
