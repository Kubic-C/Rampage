#pragma once

#include "../commondef.hpp"
#include "../ecs/id.hpp"

RAMPAGE_START

/**
 * Tracks ID remapping during deserialization.
 *
 * When loading saved data, original IDs may conflict with current world state.
 * IdRemapper handles three scenarios:
 * 1. **Reuse**: Original ID is available → use it
 * 2. **Allocate**: Original ID is taken → generate new ID
 * 3. **Lookup**: Query the remapped ID at any time
 *
 * Template is generic over any ID type (EntityId, InventoryId, AssetId, etc.)
 *
 * Usage:
 * ```cpp
 * IdRemapper<EntityId> remapper;
 * EntityId savedId = 42;
 * EntityId loadedId = remapper.apply(savedId, world.m_idMgr);
 * // loadedId is either 42 (if available) or a new allocated ID
 * ```
 */
template <typename IdType>
class IdRemapper {
public:
  IdRemapper() = default;

  /**
   * Apply remapping for a single ID.
   *
   * Strategy:
   * - If oldId is not taken in allocator, reserve it and return oldId
   * - If oldId is taken, allocate a new ID and return it
   * - Record the mapping internally
   *
   * @param oldId The ID from the saved data
   * @param allocator The IdManager that tracks current state
   * @return The ID to use during load (either oldId or a new allocation)
   */
  IdType apply(IdType oldId, IdManager<IdType>& allocator) {
    // If the original ID is already taken, generate a new one
    if (allocator.exists(oldId)) {
      IdType newId = allocator.generate();
      m_mapping[oldId] = newId;
      return newId;
    }

    // Original ID is available, reserve and use it
    allocator.ensure(oldId);
    m_mapping[oldId] = oldId;
    return oldId;
  }

  /**
   * Look up the remapped ID for an original ID.
   *
   * @param originalId The ID from saved data
   * @return The mapped ID (loaded ID)
   * @throws std::out_of_range if originalId was never remapped
   */
  IdType get(IdType originalId) const {
    return m_mapping.at(originalId);
  }

  /**
   * Check if an ID has been remapped.
   *
   * @param originalId The ID from saved data
   * @return true if this ID was processed via apply()
   */
  bool has(IdType originalId) const {
    return m_mapping.contains(originalId);
  }

  /**
   * Get the underlying mapping (for debugging/analysis).
   *
   * @return const reference to old→new ID mappings
   */
  const Map<IdType, IdType>& getMappings() const {
    return m_mapping;
  }

  /**
   * Get count of remapped IDs.
   *
   * @return Number of IDs that have been processed
   */
  size_t getRemappedCount() const {
    return m_mapping.size();
  }

  /**
   * Clear all remappings (for sequential loads).
   *
   * If loading multiple save files in sequence, call this between loads
   * to prevent stale mappings from interfering.
   */
  void reset() {
    m_mapping.clear();
  }

private:
  /**
   * Maps original IDs (from save file) to loaded IDs (in current world).
   *
   * entry.first = ID from serialized data
   * entry.second = ID allocated in current world
   *
   * If first == second, the original ID was reused (no conflict).
   * If first != second, a new ID was allocated due to conflict.
   */
  Map<IdType, IdType> m_mapping;
};

RAMPAGE_END
