#include "ref.hpp"
#include "entityPtr.hpp"

RAMPAGE_START

Ref::Ref(IWorldPtr world, EntityId id, ComponentId comp) : m_world(world), m_entity(id), m_comp(comp) {}

Ref::Ref(EntityPtr entity, ComponentId comp) : m_world(entity.world()), m_entity(entity), m_comp(comp) {}

EntityPtr Ref::getEntity() {
  return m_world->getEntity(m_entity);
}

void* Ref::get() {
  assert(m_world->has(m_entity, m_comp));
  IPool* pool = m_world->getPool(m_comp);
  assert(pool);
  void* comp = pool->get(m_entity);
  assert(comp);

  return comp;
}

RAMPAGE_END
