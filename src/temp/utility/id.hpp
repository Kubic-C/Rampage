#pragma once

#include "base.hpp"

template <typename IdType>
class IdManager {
  public:
  static constexpr IdType maxValidId = std::numeric_limits<IdType>::max();
  static constexpr IdType invalidId = 0;

  // set the last biggest id
  void setLastId(uint32_t lastId) { m_lastId = lastId; }

  void setLocalRange(IdType startingId) {
    m_startingLocalRange = startingId;
    m_lastLocalId = m_startingLocalRange;
  }

  void enableRangeCheck(bool enable) { m_enableRangeCheck = enable; }

  bool idLocked(IdType id) { return m_enableRangeCheck && exists(id); }

  IdType generate() {
    IdType newId = 0;

    if (m_enableRangeCheck) {
      if (!m_oldLocalIds.empty()) {
        newId = m_oldLocalIds.back();
        m_oldLocalIds.pop_back();
      } else {
        newId = ++m_lastLocalId;
      }

      assert(newId >= m_startingLocalRange && "Global Ids overlapping with local ones!");
    } else {
      if (!m_oldIds.empty()) {
        newId = m_oldIds.back();
        m_oldIds.pop_back();
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
      m_oldLocalIds.push_back(id);
    } else {
      m_oldIds.push_back(id);
    }
  }

  bool exists(IdType id) const { return m_ids.contains(id); }

  // Returns the biggest id given out (Applies only for global ids)
  IdType maxId() const { return m_lastId; }

  size_t count() const { return m_ids.size(); }

  bool validRange(IdType id) {
    if (!m_enableRangeCheck)
      return true;

    return id >= m_startingLocalRange;
  }

  private:
  bool m_enableRangeCheck = false;
  IdType m_startingLocalRange = maxValidId;

  IdType m_lastLocalId = 0;
  IdType m_lastId = 0;

  Set<IdType> m_ids;

  std::vector<IdType> m_oldIds;
  std::vector<IdType> m_oldLocalIds;
};

/**
 * Runtime generated IDs, for static compile time-types.
 */
template <typename T>
class StaticId {
public:
  template <typename X>
  static int id() {
    static int id = nextID();
    return id;
  }

private:
  static int nextID() {
    static int counter = 0;
    return counter++;
  }
};