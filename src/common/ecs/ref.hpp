#pragma once

#include "iworld.hpp"

RAMPAGE_START

class EntityPtr;

template <typename T>
class RefT;

// A safe-access version for components
class Ref {
public:
  Ref(IWorldPtr world, EntityId id, ComponentId comp);
  Ref(EntityPtr entity, ComponentId comp);

  void* get();

  template <typename T>
  RefT<T> cast();

  IWorldPtr getWorld() {
    return m_world;
  }

  EntityPtr getEntity();

private:
  IWorldPtr m_world;
  EntityId m_entity;
  ComponentId m_comp;
};

template <typename T>
class RefT : protected Ref {
public:
  RefT(IWorldPtr world, EntityId id) : Ref(world, id, world->component<T>()) {}

  T* operator->() {
    return reinterpret_cast<T*>(get());
  }

  T& operator*() {
    return *reinterpret_cast<T*>(get());
  }

  T copy() {
    return *reinterpret_cast<T*>(get());
  }
};

template <typename T>
RefT<T> Ref::cast() {
  assert(m_world->component<T>() == m_comp);
  return RefT<T>(m_world, m_entity);
}

RAMPAGE_END
