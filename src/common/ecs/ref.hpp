#pragma once

#include "world.hpp"

RAMPAGE_START

template<typename T>
class RefT;

// A safe-access version for components
class Ref {
public:
  Ref(EntityWorld& world, EntityId id, ComponentId comp);
  Ref(Entity entity, ComponentId comp);

  void* get();

  template<typename T>
  RefT<T> cast();

private:
  EntityWorld& m_world;
  EntityId m_entity;
  ComponentId m_comp;
};

template <typename T>
class RefT : protected Ref {
public:
  RefT(EntityWorld& world, EntityId id) : Ref(world, id, world.component<T>()) {}

  T* operator->() { return reinterpret_cast<T*>(get()); }

  T& operator*() { return *reinterpret_cast<T*>(get()); }

  T copy() { return *reinterpret_cast<T*>(get()); }
};

template<typename T>
RefT<T> Ref::cast() {
  assert(m_world.component<T>() == m_comp);
  return RefT<T>(m_world, m_entity);
}

RAMPAGE_END