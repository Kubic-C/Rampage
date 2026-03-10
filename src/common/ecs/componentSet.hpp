#pragma once

#include "id.hpp"

RAMPAGE_START

namespace alloc {
  using ComponentSetAllocator = boost::pool_allocator<ComponentId>;
  static ComponentSetAllocator componentSetAllocator;
}

class ComponentSetBuilder;

class ComponentSet {
  friend class ComponentSetBuilder;

public:
  ComponentSet() = default;
  ComponentSet(const ComponentSet&) = default;
  ComponentSet(ComponentSet&&) = default;

  ComponentSet(const std::vector<ComponentId>& ids);
  ComponentSet(const std::vector<ComponentId, alloc::ComponentSetAllocator>& ids);
  ComponentSet(const std::initializer_list<ComponentId>& ids);

  const ComponentSet add(ComponentId id) const;
  const ComponentSet remove(ComponentId id) const;
  bool has(ComponentId id) const;
  // Is this set a superset of other set
  bool superset(const ComponentSet& set) const;
  // Is this set a subset of other set
  bool subset(const ComponentSet& set) const;
  ComponentSetId getSetId() const;
  const std::vector<ComponentId, alloc::ComponentSetAllocator>& list() const;

  ComponentSet& operator=(const ComponentSet& other) = default;

  bool operator==(const ComponentSet& other) const {
    return getSetId() == other.getSetId();
  }

protected:
  std::vector<ComponentId, alloc::ComponentSetAllocator> m_comps;
};

class ComponentSetBuilder {
public:
  static const ComponentSet empty;

  ComponentSetBuilder() = default;
  ComponentSetBuilder(const ComponentSetBuilder&) = default;
  ComponentSetBuilder(ComponentSetBuilder&&) = default;

  ComponentSetBuilder(const ComponentSet& ids);
  ComponentSetBuilder(const std::initializer_list<ComponentId>& ids);

  const void add(ComponentId id);
  const void add(const ComponentSet& set);
  const void remove(ComponentId id);
  const void remove(const ComponentSet& set);
  bool has(ComponentId id) const;
  // Is this set a superset of other set
  bool superset(const ComponentSet& set) const;
  // Is this set a subset of other set
  bool subset(const ComponentSet& set) const;
  ComponentSetId getSetId() const;
  const std::vector<ComponentId, alloc::ComponentSetAllocator>& list() const;
  std::vector<ComponentId, alloc::ComponentSetAllocator>& list();
  ComponentSet build() const;

protected:
  std::vector<ComponentId, alloc::ComponentSetAllocator> m_comps;
};

RAMPAGE_END

namespace boost {
  template <>
  struct hash<rmp::ComponentSet> {
    size_t operator()(const rmp::ComponentSet& set) const {
      return set.getSetId();
    }
  };
} // namespace boost

namespace std {
  template <>
  struct hash<rmp::ComponentSet> {
    size_t operator()(const rmp::ComponentSet& set) const {
      return set.getSetId();
    }
  };
} // namespace std
