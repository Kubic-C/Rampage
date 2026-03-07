#pragma once

#include "../commondef.hpp"

RAMPAGE_START

template <typename IdType>
class IdManager {
public:
  static constexpr IdType maxValidId = std::numeric_limits<IdType>::max();
  static constexpr IdType invalidId = IdType(0);

  // set the last biggest id
  void setLastId(u32 lastId) {
    m_lastId = lastId;
  }

  void setLocalRange(IdType startingId) {
    m_startingLocalRange = startingId;
    m_lastLocalId = m_startingLocalRange;
  }

  void enableRangeCheck(bool enable) {
    m_enableRangeCheck = enable;
  }

  bool idLocked(IdType id) {
    return m_enableRangeCheck && exists(id);
  }

  IdType generate() {
    IdType newId = IdType(0);

    if (m_enableRangeCheck) {
      if (!m_oldLocalIds.empty()) {
        newId = *m_oldLocalIds.begin();
      } else {
        newId = ++m_lastLocalId;
      }

      assert(newId >= m_startingLocalRange && "Global Ids overlapping with local ones!");
    } else {
      if (!m_oldIds.empty()) {
        newId = *m_oldIds.begin();
      } else {
        newId = ++m_lastId;
      }
    }

    ensure(newId);

    return newId;
  }

  // Returns true if the id previously existed, false if it did not.
  // ENSURES "id" is reserved. Must only be used when disabling the range check
  bool ensure(IdType id) {
    if (!validRange(id)) {
      std::cout << "Attempt to ensure an id that is below local range\n";
      abort();
    }

    if (exists(id))
      return true;

    m_ids.insert(id);

    if (m_lastId < id) {
      m_lastId = id;
    }

    // Remove from recycle lists so generate() won't hand out this ID again
    if(m_oldIds.contains(id))
      m_oldIds.erase(id);
    if(m_oldLocalIds.contains(id))
      m_oldLocalIds.erase(id);

    return false;
  }

  void destroy(IdType id) {
    if (!validRange(id)) {
      std::cout << "Attempt to ensure an id that is below local range\n";
      abort();
    }

    if (!exists(id)) {
      std::cout << "Attempt to destroy an ID that does not exist\n";
      abort();
    }

    m_ids.erase(id);

    if (id >= m_startingLocalRange) {
      m_oldLocalIds.insert(id);
    } else {
      m_oldIds.insert(id);
    }
  }

  bool exists(IdType id) const {
    return m_ids.contains(id);
  }

  // Returns the biggest id given out (Applies only for global ids)
  IdType maxId() const {
    return m_lastId;
  }

  size_t count() const {
    return m_ids.size();
  }

  bool validRange(IdType id) {
    if (!m_enableRangeCheck)
      return true;

    return id >= m_startingLocalRange;
  }

private:
  bool m_enableRangeCheck = false;
  IdType m_startingLocalRange = maxValidId;

  IdType m_lastLocalId = IdType(0);
  IdType m_lastId = IdType(0);

  Set<IdType> m_ids;
  Set<IdType> m_oldIds;
  Set<IdType> m_oldLocalIds;
};

using EntityId = u32;
using ComponentId = u16;
using ComponentSetId = u64;
using EventTypeId = u32;
using ContextId = u32;
static constexpr EntityId NullEntityId = 0;
static constexpr ComponentId NullComponentId = 0;

/**
 * Ensures a stable, incremental ID, regardless of Compilation Unit.
 * Works across dynamic libraries.
 *
 * The main object is owned by the host process and then
 * shared among its processes.
 */
template <typename IntT>
class StaticIdManager {
  template <typename T>
  constexpr static std::string_view typeName() {
#if defined(_MSC_VER)
    return __FUNCSIG__; // MSVC
#else
    return __PRETTY_FUNCTION__; // GCC/Clang
#endif
  }

public:
  template <typename T>
  IntT id() {
    static IntT id = nextID<T>(); // Initialized once per unique build.
    return id;
  }

private:
  // This is called once per initialization per type per seperate compilation units.. (it works across DLLs)
  template <typename T>
  IntT nextID() {
    size_t idHash = std::hash<std::string_view>{}(typeName<T>());
    auto it = m_registry.find(idHash);
    if (it != m_registry.end()) {
      return it->second;
    }

    int newId = ++m_counter;
    m_registry[idHash] = newId;
    return newId;
  }

public:
  IntT m_counter = 0;
  Map<size_t, IntT> m_registry;
};

template<typename Tag, typename IntT = u32>
class StrongId {
  IntT m_value;
public:
  using IntegerType = IntT;

  explicit constexpr StrongId(IntT v = 0) : m_value(v) {}
  constexpr IntT value() const { return m_value; }

  bool operator==(const StrongId& other) const { return m_value == other.m_value; }
  bool operator!=(const StrongId& other) const { return m_value != other.m_value; }
  bool operator<(const StrongId& other) const { return m_value < other.m_value; }
  bool operator>(const StrongId& other) const { return m_value > other.m_value; }
  bool operator<=(const StrongId& other) const { return m_value <= other.m_value; }
  bool operator>=(const StrongId& other) const { return m_value >= other.m_value; } 

  StrongId operator++() { return StrongId(++m_value); }
  StrongId operator++(int) { StrongId temp(*this); ++m_value; return temp; }
};

class IdMapper {
public:
  EntityId resolve(EntityId originalId) const {
    auto it = m_idMap.find(originalId);
    if (it != m_idMap.end())
      return it->second;
    return originalId;
  }

  void add(EntityId originalId, EntityId mappedId) {
    m_idMap[originalId] = mappedId;
  }

private:
  OpenMap<EntityId, EntityId> m_idMap;
};

RAMPAGE_END
