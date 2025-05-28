#include "ref.hpp"

#include "entity.hpp"

Ref::Ref(EntityWorld& world, EntityId id, ComponentId comp) : m_world(world), m_entity(id), m_comp(comp) {}

Ref::Ref(Entity entity, ComponentId comp) : m_world(entity.world()), m_entity(entity), m_comp(comp) {}

void* Ref::get() {
  assert(Entity(m_world, m_entity).has(m_comp));

  return reinterpret_cast<void*>(m_world.getPool(m_comp)->get(m_entity));
}
