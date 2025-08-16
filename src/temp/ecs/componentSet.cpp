#include "componentSet.hpp"

ComponentSet::ComponentSet(const std::vector<ComponentId>& ids) : m_comps(ids) {
  std::sort(m_comps.begin(), m_comps.end());
}

ComponentSet::ComponentSet(const std::initializer_list<ComponentId>& ids) : m_comps(ids) {
  std::sort(m_comps.begin(), m_comps.end());
}

const ComponentSet ComponentSet::add(ComponentId id) const {
  ComponentSet addSet = *this;

  auto it = std::lower_bound(addSet.m_comps.begin(), addSet.m_comps.end(), id);
  if (it == addSet.m_comps.end() || *it != id)
    addSet.m_comps.insert(it, id);

  return addSet;
}

const ComponentSet ComponentSet::remove(ComponentId id) const {
  ComponentSet removeSet = *this;

  auto it = std::lower_bound(removeSet.m_comps.begin(), removeSet.m_comps.end(), id);
  if (it != removeSet.m_comps.end() && *it == id)
    removeSet.m_comps.erase(it);

  return removeSet;
}

bool ComponentSet::has(ComponentId id) const {
  if (m_comps.empty())
    return false;

  auto it = std::lower_bound(m_comps.begin(), m_comps.end(), id);
  if (it != m_comps.end() && *it == id)
    return true;
  else
    return false;
}

bool ComponentSet::superset(const ComponentSet& set) const {
  for (ComponentId otherId : set.m_comps)
    if (!has(otherId))
      return false;

  return true;
}

bool ComponentSet::subset(const ComponentSet& set) const {
  for (ComponentId thisId : m_comps)
    if (!set.has(thisId))
      return false;

  return true;
}

ComponentSetId ComponentSet::getSetId() const {
  ComponentSetId hashValue = 0;
  for (const auto& el : m_comps) {
    hashValue ^= std::hash<ComponentId>()(el) + 0x9e3779b9 + (hashValue << 6) + (hashValue >> 2);
  }
  return hashValue;
}

const std::vector<ComponentId>& ComponentSet::list() const { return m_comps; }

ComponentSetBuilder::ComponentSetBuilder(const ComponentSet& ids) : m_comps(ids.list()) {}

ComponentSetBuilder::ComponentSetBuilder(const std::initializer_list<ComponentId>& ids) : m_comps(ids) {
  std::sort(m_comps.begin(), m_comps.end());
}

const void ComponentSetBuilder::add(ComponentId id) {
  auto it = std::lower_bound(m_comps.begin(), m_comps.end(), id);
  if (it == m_comps.end() || *it != id)
    m_comps.insert(it, id);
}

const void ComponentSetBuilder::add(const ComponentSet& set) {
  m_comps.reserve(set.list().size());
  for (ComponentId id : set.list())
    add(id);
}

const void ComponentSetBuilder::remove(ComponentId id) {
  auto it = std::lower_bound(m_comps.begin(), m_comps.end(), id);
  if (it != m_comps.end() && *it == id)
    m_comps.erase(it);
}

const void ComponentSetBuilder::remove(const ComponentSet& set) {
  for (ComponentId id : set.list())
    remove(id);
}

bool ComponentSetBuilder::has(ComponentId id) const {
  if (m_comps.empty())
    return false;

  auto it = std::lower_bound(m_comps.begin(), m_comps.end(), id);
  if (it != m_comps.end() && *it == id)
    return true;
  else
    return false;
}

bool ComponentSetBuilder::superset(const ComponentSet& set) const {
  for (ComponentId otherId : set.m_comps)
    if (!has(otherId))
      return false;

  return true;
}

bool ComponentSetBuilder::subset(const ComponentSet& set) const {
  for (ComponentId thisId : m_comps)
    if (!set.has(thisId))
      return false;

  return true;
}

ComponentSetId ComponentSetBuilder::getSetId() const {
  ComponentSetId hashValue = 0;
  for (const auto& el : m_comps) {
    hashValue ^= std::hash<ComponentId>()(el) + 0x9e3779b9 + (hashValue << 6) + (hashValue >> 2);
  }
  return hashValue;
}

const std::vector<ComponentId>& ComponentSetBuilder::list() const { return m_comps; }

ComponentSet ComponentSetBuilder::build() const { return ComponentSet(m_comps); }
