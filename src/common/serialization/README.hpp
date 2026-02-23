#pragma once

/**
 * SERIALIZATION ARCHITECTURE GUIDE
 *
 * Overview:
 * =========
 * This directory contains a clean, centralized serialization system designed to handle
 * ID persistence and conflict resolution. It's built around the IdRemapper pattern.
 *
 *
 * COMPONENTS
 * ==========
 *
 * 1. IdRemapper<IdType> [idRemapper.hpp]
 *    - Generic template for any ID type (EntityId, InventoryId, AssetId, etc.)
 *    - Core logic: tries to reuse original IDs, allocates new ones on conflict
 *    - Tracks mappings for later lookup
 *    - Usage:
 *      IdRemapper<EntityId> remapper;
 *      EntityId loadedId = remapper.apply(savedId, world->m_idMgr);
 *
 * 2. SaveData [saveData.hpp]
 *    - Serializable data structures (SerializedEntity, SerializedInventory)
 *    - Intermediate representation between disk and world
 *    - Knows how to apply remapping to itself
 *    - Usage:
 *      SaveData data = loadFrom Disk();
 *      data.applyRemapping(entityRemapper, inventoryRemapper);
 *
 * 3. SaveCoordinator [saveCoordinator.hpp + .cpp]
 *    - Orchestrates the entire load/save process
 *    - Single place for all serialization concerns
 *    - Usage:
 *      SaveCoordinator coordinator(world, invMgr);
 *      if (coordinator.loadGame("save.json")) { ... }
 *      if (coordinator.saveGame("save.json")) { ... }
 *
 *
 * LOAD FLOW
 * =========
 *
 * File on Disk (IDs: EntityId=42, InventoryId=5)
 *         ↓
 *  [1] LoadFromDisk()
 *         ↓
 *    SaveData (original IDs preserved)
 *         ↓
 *  [2] ResolveConflicts()
 *      - Check: Is EntityId 42 already taken in world?
 *      - YES → allocate new ID (say, 156), record mapping 42→156
 *      - NO → reserve ID 42, record mapping 42→42
 *         ↓
 *      EntityId Remapper: { 42→156 }
 *      InventoryId Remapper: { 5→5 }
 *         ↓
 *  [3] ApplyRemapping()
 *      Scan SaveData:
 *      - Update entity.id: 42 → 156
 *      - Update inventory items: item entity 42 → 156
 *      - Update inventory.id: 5 → 5
 *         ↓
 *    SaveData (remapped IDs)
 *         ↓
 *  [4] ApplyToWorld()
 *      - Create entities with remapped IDs
 *      - Create inventories with remapped IDs
 *      - All references now point to correct objects
 *         ↓
 *    ✓ World updated successfully
 *
 *
 * DESIGN PATTERNS
 * ===============
 *
 * Pattern 1: Reuse IDs When Safe
 *   If saved EntityId 42 never existed in current world, reuse 42.
 *   Provides player experience of "stable IDs" across save/load cycles.
 *
 * Pattern 2: Single Point of Coordination
 *   SaveCoordinator is the only place handling the orchestration.
 *   Makes it easy to add new serialization concerns later.
 *
 * Pattern 3: Generic ID Remapper
 *   Works with any ID type thanks to templates.
 *   No code duplication for EntityId vs InventoryId vs custom IDs.
 *
 * Pattern 4: Separate Concerns
 *   - IdRemapper: just track/allocate IDs
 *   - SaveData: just hold serializable data
 *   - SaveCoordinator: just orchestrate the flow
 *
 *
 * EXTENDING THE SYSTEM
 * ====================
 *
 * Adding a new ID type (e.g., TileId):
 *   - No changes to IdRemapper (already generic)
 *   - Add SerializedTile to SaveData
 *   - Add remapping logic to SaveData::applyRemapping()
 *   - Add loading logic to SaveCoordinator::applyTilesToWorld()
 *
 * Adding new metadata (version, player name, etc.):
 *   - Add fields to SaveData struct
 *   - Add serialization in saveGame()
 *   - Add deserialization in loadFromDisk()
 *
 * Changing serialization format (JSON → Binary):
 *   - Only SaveCoordinator::loadFromDisk() and saveToDisk() need changes
 *   - Everything else stays the same
 *
 *
 * TODO ITEMS FOR IMPLEMENTATION
 * ==============================
 *
 * 1. Implement SaveCoordinator::loadFromDisk()
 *    - Choose format (JSON using glaze, Capnproto, binary)
 *    - Parse SaveData from file
 *
 * 2. Implement SaveCoordinator::saveToDisk()
 *    - Serialize SaveData to file
 *
 * 3. Implement SaveCoordinator::applyEntitiesToWorld()
 *    - Use component serialization system to restore entities
 *    - Handle component reference remapping
 *
 * 4. Implement SaveCoordinator::applyInventoriesToWorld()
 *    - Create inventories and populate with items
 *    - Reconstruct UI widgets for inventory
 *
 * 5. Add clear() method to IWorldPtr
 *    - Need way to destroy all entities atomically for "New Game"
 *
 * 6. Expose InventoryManager::m_idMgr (or create getter)
 *    - Currently SaveCoordinator can't access it
 *    - Either make it public, add getter, or pass IdManager to SaveCoordinator
 *
 *
 * EXAMPLE USAGE
 * =============
 *
 * In your game initialization:
 *
 *     IWorldPtr world(host);
 *     InventoryManager invMgr(world);
 *     SaveCoordinator saver(world, invMgr);
 *
 *     // Load on startup
 *     if (saver.loadGame("last_save.json")) {
 *       // Game state restored with conflict resolution handled
 *       playGame(world, invMgr);
 *     } else {
 *       // Start new game
 *       initializeNewGame(world, invMgr);
 *     }
 *
 *     // Save before quit
 *     saver.saveGame("last_save.json");
 *
 *
 * CONFLICT RESOLUTION SCENARIOS
 * ==============================
 *
 * Scenario A: Fresh Start (No Conflict)
 *   - Saved game has EntityId 42
 *   - World is empty (42 doesn't exist)
 *   - Result: Entity loaded as ID 42 ✓
 *
 * Scenario B: Continue + New Game (Conflict)
 *   - Saved game has EntityId 42
 *   - World already has EntityId 42 (from previous play)
 *   - Result: Entity loaded as new ID (156), all references remapped ✓
 *
 * Scenario C: Loading Content Additively
 *   - Saved game represents a dungeon level
 *   - World has existing entities from overworld
 *   - Result: Dungeon entities loaded with new IDs, no conflicts ✓
 *
 * Scenario D: Multiple Inventories
 *   - Saved game has 3 inventories: IDs 1, 2, 3
 *   - World reuses IDs 1 and 3, conflicts on 2
 *   - Result: Loaded as IDs 1, 4 (new), 3 ✓
 *   - All item references within inventories remapped correctly
 *
 */
