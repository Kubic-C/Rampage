#pragma once

#include "world.hpp"

// A safe-access version for components
class Ref {
  public:
  Ref(EntityWorld& world, EntityId id, ComponentId comp);
  Ref(Entity entity, ComponentId comp);

  void* get();

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
