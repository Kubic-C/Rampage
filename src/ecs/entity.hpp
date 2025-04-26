#pragma once

#include "ref.hpp"

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
  bool has(ComponentId compId);
  EntityWorld& world();
  void enable();
  void disable();
  Entity clone();

  template<typename T>
  void add() {
    add(m_world.component<T>());
  }

  template<typename T>
  void remove() {
    remove(m_world.component<T>());
  }

  template<typename T>
  RefT<T> get() {
    return RefT<T>(m_world, m_id);

  }
  template<typename T>
  bool has() {
    return has(m_world.component<T>());
  }

  operator EntityId() const {
    return m_id;
  }

private:
  EntityWorld& m_world;
  EntityId m_id;
};

inline void* entityToB2Data(EntityId id) {
  return (void*)id;
}

inline Entity b2DataToEntity(EntityWorld& world, void* vp) {
  return world.get((EntityId)vp);
}

template<typename T, typename ... Params>
Entity EntityWorld::addModule(Params&& ... args) {
  component<ModuleType<T>>();

  Entity moduleEntity = create();
  moduleEntity.template add<ModuleData>();
  moduleEntity.template add<ModuleType<T>>();
  RefT<ModuleData> moduleT = moduleEntity.template get<ModuleData>();
  moduleT->m_module = std::make_shared<T>(*this, args...);

  disable(moduleEntity);

  return moduleEntity;
}