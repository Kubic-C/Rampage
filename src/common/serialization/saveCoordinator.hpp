#pragma once

#include "../commondef.hpp"
#include "../ecs/world->hpp"
#include "idRemapper.hpp"
#include "saveData.hpp"

RAMPAGE_START

class InventoryManager;  // Forward declare

/**
 * Orchestrates the entire save/load lifecycle.
 *
 * Handles:
 * - Loading SaveData from disk
 * - Detecting and resolving ID conflicts
 * - Remapping all references atomically
 * - Applying loaded state to IWorldPtr and InventoryManager
 *
 * This is the central, single place where all serialization concerns are coordinated.
 */
class SaveCoordinator {
public:
  SaveCoordinator(IWorldPtr world, InventoryManager& invMgr)
      : m_world(world), m_invMgr(invMgr) {}

  /**
   * Load a game from disk into the world->
   *
   * Process:
   * 1. Load SaveData from file (format: JSON, binary, etc.)
   * 2. Detect conflicts between saved IDs and current world state
   * 3. Allocate new IDs where needed
   * 4. Remap all references in SaveData
   * 5. Apply remapped data to IWorldPtr and InventoryManager
   * 6. Return success/failure status
   *
   * @param filepath Path to save file
   * @param clearExisting If true, destroy all current entities before loading
   * @return true if load succeeded, false otherwise
   */
  bool loadGame(const std::string& filepath, bool clearExisting = true);

  /**
   * Save current game state to disk.
   *
   * Process:
   * 1. Serialize all entities from IWorldPtr
   * 2. Serialize all inventories from InventoryManager
   * 3. Create SaveData structure with current IDs (no remapping needed)
   * 4. Write to disk (format: JSON, binary, etc.)
   *
   * @param filepath Path to save file
   * @param metadata Additional metadata (player name, difficulty, etc.)
   * @return true if save succeeded, false otherwise
   */
  bool saveGame(const std::string& filepath, const Map<std::string, std::string>& metadata = {});

private:
  /**
   * Load SaveData from disk.
   *
   * Actual format depends on your choice:
   * - JSON (via nlohmann/json or glaze)
   * - Binary (custom format)
   * - Capnproto (since you use it in the codebase)
   *
   * @param filepath Path to save file
   * @return SaveData structure, or throws on parse error
   */
  SaveData loadFromDisk(const std::string& filepath);

  /**
   * Write SaveData to disk.
   *
   * @param filepath Path to save file
   * @param data SaveData to serialize
   * @return true if write succeeded
   */
  bool saveToDisk(const std::string& filepath, const SaveData& data);

  /**
   * Resolve ID conflicts by creating remapping.
   *
   * Check which IDs from SaveData conflict with current world state,
   * allocate new IDs where needed, build remapping tables.
   *
   * @param data SaveData (will be modified with new IDs)
   * @param entityRemapper Output: maps EntityId
   * @param inventoryRemapper Output: maps InventoryId
   */
  void resolveConflicts(SaveData& data, IdRemapper<EntityId>& entityRemapper,
                        IdRemapper<InventoryId>& inventoryRemapper);

  /**
   * Apply remapped SaveData to IWorldPtr.
   *
   * Creates entities with their serialized components using the remapped IDs.
   *
   * @param data SaveData with remapped IDs
   */
  void applyEntitiesToWorld(const SaveData& data);

  /**
   * Apply remapped SaveData to InventoryManager.
   *
   * Creates inventories with items using the remapped IDs.
   *
   * @param data SaveData with remapped IDs
   */
  void applyInventoriesToWorld(const SaveData& data);

  IWorldPtr m_world;
  InventoryManager& m_invMgr;
};

RAMPAGE_END
