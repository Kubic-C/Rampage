#pragma once

#include "ref.hpp"

RAMPAGE_START

class Entity {
public:
  explicit Entity(EntityWorld& world);
  Entity(EntityWorld& world, EntityId id);
  Entity& operator=(Entity other);

  NODISCARD EntityId id() const;
  NODISCARD bool exists() const;
  NODISCARD bool alive() const;
  NODISCARD bool isNull() const;
  NODISCARD const ComponentSet& set() const;
  void add(ComponentId compId, bool emit = true);
  void remove(ComponentId compId, bool emit = true);
  void add(const ComponentSet& comps, bool emit = true);
  void remove(const ComponentSet& comps, bool emit = true);
  NODISCARD Ref get(ComponentId compId);
  NODISCARD bool has(ComponentId compId) const;
  NODISCARD EntityWorld& world();
  NODISCARD bool isEnabled() const;
  void enable();
  void disable();
  NODISCARD Entity clone() const;
  void copyInto(EntityId id);

  template <typename T>
  void add() {
    add(m_world->component<T>());
  }

  template <typename T>
  void remove() {
    remove(m_world->component<T>());
  }

  template <typename T>
  RefT<T> get() {
    return RefT<T>(*m_world, m_id);
  }

  template <typename T>
  bool has() const {
    return has(m_world->component<T>());
  }

  operator EntityId() const {
    return m_id;
  }

private:
  EntityWorld* m_world;
  EntityId m_id;
};

inline void* entityToB2Data(const EntityId id) {
  return reinterpret_cast<void*>(id);
}

inline Entity b2DataToEntity(EntityWorld& world, void* vp) {
  return world.get(static_cast<EntityId>(reinterpret_cast<uintptr_t>(vp)));
}

template <typename EventType>
void EntityWorld::observe(ComponentId comp, const ComponentSet& with, ObserverCallback callback) {
  m_observers[component<EventType>()][comp].emplace_back(ObserverData(findOrCreateSet(with), callback));
}

template <typename EventType>
void EntityWorld::emit(EntityId id, ComponentId comp) {
  for (ObserverData& observer : m_observers[component<EventType>()][comp]) {
    if (getEntitySet(id)->superset(*observer.with)) {
      observer.callback(get(id));
    }
  }
}

RAMPAGE_END
