#pragma once

#include "world.hpp"
#include "entity.hpp"

class System {
  friend class EntityWorld;

protected:
  System(const EntityIterator& it, const EntityWorld::SystemFunc& func)
    : m_it(it), m_func(func) {
  }
public:
  void run(float deltaTime = 0.0f) {
    EntityWorld& world = m_it.getWorld();

    m_it.reset();
    world.beginDefer();
    while (m_it.hasNext()) {
      m_func(m_it.next(), deltaTime);
    }
    world.endDefer();
  }

private:
  EntityIterator m_it;
  EntityWorld::SystemFunc m_func;
};