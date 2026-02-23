#include "saveCoordinator.hpp"

#include "../ecs/world->hpp"
#include "inventory.hpp"  // For InventoryManager

RAMPAGE_START

bool SaveCoordinator::loadGame(const std::string& filepath, bool clearExisting) {
  try {
    // Step 1: Load from disk
    SaveData data = loadFromDisk(filepath);

    // Step 2: Optionally clear existing world
    if (clearExisting) {
      // Destroy all entities (implementation depends on IWorldPtr API)
      // For now, this is left to the user to implement based on their needs
      // Example: while (IWorldPtr has entities) destroy oldest
    }

    // Step 3: Detect conflicts and allocate new IDs
    IdRemapper<EntityId> entityRemapper;
    IdRemapper<InventoryId> inventoryRemapper;
    resolveConflicts(data, entityRemapper, inventoryRemapper);

    // Step 4: Remap all references in SaveData
    data.applyRemapping(entityRemapper, inventoryRemapper);

    // Step 5: Apply to world (atomic operation, all-or-nothing)
    applyEntitiesToWorld(data);
    applyInventoriesToWorld(data);

    return true;
  } catch (const std::exception& e) {
    std::cerr << "Failed to load game: " << e.what() << std::endl;
    return false;
  }
}

bool SaveCoordinator::saveGame(const std::string& filepath,
                               const Map<std::string, std::string>& metadata) {
  try {
    SaveData data;
    data.gameVersion = "1.0";  // TODO: Define versioning strategy
    // data.timestamp = getCurrentTimestamp();  // TODO: Implement this

    // Serialize all entities (implementation depends on component serialization)
    // Example: for each entity in IWorldPtr, create SerializedEntity
    // for (Entity e : world->getWithDisabled(...)) {
    //   SerializedEntity se = serializeEntity(e);
    //   data.entities.push_back(se);
    // }

    // Serialize all inventories
    // for (InventoryId id : invMgr.getAllInventoryIds()) {
    //   Inventory inv = invMgr.getInventory(id);
    //   SerializedInventory si = serializeInventory(inv);
    //   data.inventories.push_back(si);
    // }

    return saveToDisk(filepath, data);
  } catch (const std::exception& e) {
    std::cerr << "Failed to save game: " << e.what() << std::endl;
    return false;
  }
}

void SaveCoordinator::resolveConflicts(SaveData& data, IdRemapper<EntityId>& entityRemapper,
                                       IdRemapper<InventoryId>& inventoryRemapper) {
  // Process all entity IDs
  for (auto& entity : data.entities) {
    entity.id = entityRemapper.apply(entity.id, m_world->m_idMgr);
  }

  // Process all inventory IDs
  for (auto& inventory : data.inventories) {
    inventory.id = inventoryRemapper.apply(inventory.id, m_invMgr.m_idMgr);
  }
}

void SaveCoordinator::applyEntitiesToWorld(const SaveData& data) {
  // TODO: Implement based on your component serialization system
  // For each SerializedEntity:
  // 1. Create entity with explicit ID: m_world->create(entity.id)
  // 2. Deserialize components: for each component, entity.add<T>() and restore data
  // 3. Handle component references: remap any EntityIds in components

  // Example skeleton:
  // for (const auto& serializedEntity : data.entities) {
  //   Entity e = m_world->create(serializedEntity.id);
  //   deserializeComponentsInto(e, serializedEntity.componentData);
  // }
}

void SaveCoordinator::applyInventoriesToWorld(const SaveData& data) {
  // TODO: Implement based on actual InventoryManager API
  // For each SerializedInventory:
  // 1. Create inventory with explicit ID
  // 2. Populate slots: addItem(itemEntityId, position, stackCount)

  // Example skeleton:
  // for (const auto& serializedInv : data.inventories) {
  //   Inventory inv = m_invMgr.createInventory(serializedInv.name, ...);
  //   for (const auto& [pos, stack] : serializedInv.items) {
  //     inv.addItem(stack.item, pos, stack.stackCount);
  //   }
  // }
}

SaveData SaveCoordinator::loadFromDisk(const std::string& filepath) {
  // TODO: Implement based on your serialization format
  // Options:
  // - JSON (if using nlohmann/json or glaze)
  // - Binary (custom format)
  // - Capnproto (since you already use it)

  // Example JSON skeleton:
  // std::ifstream file(filepath);
  // auto json = nlohmann::json::parse(file);
  // return json.get<SaveData>();

  // For now, throw to indicate not yet implemented
  throw std::runtime_error("SaveCoordinator::loadFromDisk not yet implemented");
}

bool SaveCoordinator::saveToDisk(const std::string& filepath, const SaveData& data) {
  // TODO: Implement based on your serialization format

  // Example JSON skeleton:
  // std::ofstream file(filepath);
  // auto json = nlohmann::json(data);
  // file << json.dump(2);
  // return true;

  throw std::runtime_error("SaveCoordinator::saveToDisk not yet implemented");
}

RAMPAGE_END
