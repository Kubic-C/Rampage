#pragma once

#include "ref.hpp"

RAMPAGE_START

class Entity {
  public:
  Entity(EntityWorld& world);
  Entity(EntityWorld& world, EntityId id);
  Entity& operator=(Entity other);

  EntityId id() const;
  bool exists() const;
  bool alive() const;
  bool isNull() const;
  const ComponentSet& set() const;
  void add(ComponentId compId, bool notify = true);
  void remove(ComponentId compId, bool notify = true);
  void add(const ComponentSet& comps, bool notify = true);
  void remove(const ComponentSet& comps, bool notify = true);
  Ref get(ComponentId compId);
  bool has(ComponentId compId) const;
  EntityWorld& world();
  bool isEnabled() const;
  void enable();
  void disable();
  Entity clone() const;
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

  operator EntityId() const { return m_id; }

  private:
  EntityWorld* m_world;
  EntityId m_id;
};

inline void* entityToB2Data(const EntityId id) { return reinterpret_cast<void*>(id); }

inline Entity b2DataToEntity(EntityWorld& world, void* vp) {
  return world.get(static_cast<EntityId>(reinterpret_cast<uintptr_t>(vp)));
}

RAMPAGE_END