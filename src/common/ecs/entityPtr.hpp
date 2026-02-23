#pragma once

#include "ref.hpp"

RAMPAGE_START

class EntityPtr {
public:
  explicit EntityPtr(IWorldPtr world) : m_world(world), m_id(NullEntityId) {}
  EntityPtr(const EntityPtr& other) : m_world(other.m_world), m_id(other.m_id) {}
  EntityPtr(IWorldPtr world, EntityId id) : m_world(world), m_id(id) {}
  EntityPtr& operator=(const EntityPtr& other) {
    m_world = other.m_world;
    m_id = other.m_id;
    return *this;
  }

  EntityId id() const { return m_id; }
  bool exists() const { return m_world->exists(m_id); }
  bool alive() const { return m_world->isAlive(m_id); }
  bool isNull() const { return m_id == NullEntityId; }
  const ComponentSet& set() const { return m_world->setOf(m_id); }
  void add(ComponentId compId, bool emit = true) { add(ComponentSet({compId}), emit); }
  void remove(ComponentId compId, bool emit = true) { remove(ComponentSet({compId}), emit); }
  void add(const ComponentSet& addComps, bool emit = true) { m_world->add(m_id, addComps, emit); }
  void remove(const ComponentSet& remComps, bool emit = true) { m_world->remove(m_id, remComps, emit); }
  Ref get(ComponentId compId) { return Ref(m_world, m_id, compId); }
  bool has(ComponentId compId) const { return m_world->has(m_id, compId); }
  IWorldPtr world() { return m_world; }
  bool isEnabled() const { return has<IWorld::Enabled>(); }
  void enable() { add<IWorld::Enabled>(); }
  void disable() { remove<IWorld::Enabled>(); }
  EntityPtr clone() const { return m_world->clone(m_id); }
  void copyInto(EntityId dstId) { /* TODO: implement if needed */ }

  template <typename ... Params>
  void add(bool emit = true) {
    m_world->add(m_id, m_world->set<Params...>(), emit);
  }

  template <typename ... Params>
  void remove(bool emit = true) {
    m_world->remove(m_id, m_world->set<Params...>(), emit);
  }

  template <typename T>
  RefT<T> get() {
    return RefT<T>(m_world, m_id);
  }

  template <typename T>
  bool has() const {
    return has(m_world->component<T>());
  }

  operator EntityId() const {
    return m_id;
  }

private:
  IWorldPtr m_world;
  EntityId m_id;
};

inline void* entityToB2Data(const EntityId id) {
  return reinterpret_cast<void*>(id);
}

inline EntityId b2RawDataToEntity(void* vp) {
  return static_cast<EntityId>(reinterpret_cast<uintptr_t>(vp));
}

inline EntityPtr b2DataToEntity(IWorldPtr world, void* vp) {
  return world->getEntity(b2RawDataToEntity(vp));
}


RAMPAGE_END