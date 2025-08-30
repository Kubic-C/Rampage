#include "entity.hpp"

RAMPAGE_START

Entity::Entity(EntityWorld& world) : m_world(&world), m_id(NullEntityId) {}

Entity::Entity(EntityWorld& world, EntityId id) : m_world(&world), m_id(id) {}

Entity& Entity::operator=(Entity other) {
  m_world = other.m_world;
  m_id = other.m_id;

  return *this;
}

EntityId Entity::id() const { return m_id; }

bool Entity::exists() const { return m_world->exists(m_id); }

bool Entity::alive() const { return m_world->isAlive(m_id); }

bool Entity::isNull() const { return m_id == NullEntityId; }

void Entity::add(ComponentId compId, bool notify) { add(ComponentSet({compId}), notify); }

void Entity::remove(ComponentId compId, bool notify) { remove(ComponentSet({compId}), notify); }

void Entity::add(const ComponentSet& addComps, bool notify) {
  const ComponentSet& oldSet = set();

  ComponentSetBuilder tempSet(oldSet);
  for (ComponentId compId : addComps.list()) {
    if (oldSet.has(compId))
      continue;

    IPool* pool = m_world->getPool(compId);
    tempSet.add(compId);
    if (pool)
      pool->create(m_id);
  }
  m_world->tryMoveSets(m_id, m_world->findOrCreateSet(tempSet.build()));

  if (notify && &oldSet != m_world->findOrCreateSet(tempSet.build()))
    for (ComponentId compId : addComps.list()) {
      if (oldSet.has(compId))
        continue;
      m_world->notify(EntityWorld::EventType::Add, m_id, compId);
    }
}

void Entity::remove(const ComponentSet& remComps, bool notify) {
#ifndef NDEBUG
  {
    const ComponentSet& oldSet = set();
    for (ComponentId compId : remComps.list())
      assert(oldSet.has(compId));
  }
#endif

  if (notify)
    for (ComponentId compId : remComps.list())
      m_world->notify(EntityWorld::EventType::Remove, m_id, compId);

  const ComponentSet& oldSet = set();
  ComponentSetBuilder tempSet(oldSet);
  for (ComponentId compId : remComps.list()) {
    if (!oldSet.has(compId))
      throw std::runtime_error("Set does not contain compId: " + std::to_string(compId));

    IPool* pool = m_world->getPool(compId);
    tempSet.remove(compId);
    if (pool)
      pool->destroy(m_id);
  }

  m_world->tryMoveSets(m_id, m_world->findOrCreateSet(tempSet.build()));
}

Ref Entity::get(ComponentId compId) { return Ref(*m_world, m_id, compId); }

bool Entity::has(ComponentId compId) const { return m_world->hasComponent(m_id, compId); }

EntityWorld& Entity::world() { return *m_world; }

bool Entity::isEnabled() const { return has<EntityWorld::Enabled>(); }

void Entity::enable() { add<EntityWorld::Enabled>(); }

void Entity::disable() { remove<EntityWorld::Enabled>(); }

Entity Entity::clone() const { return m_world->clone(m_id); }

void Entity::copyInto(EntityId dstId) {
  const ComponentSet& srcSet = *m_world->getEntitySet(m_id);
  Entity src = m_world->get(m_id);
  Entity dst = m_world->get(dstId);
  dst.add(srcSet);

  for (ComponentId cid : srcSet.list()) {
    auto copyCtor = m_world->m_componentCopyCtor[cid];

    if (copyCtor)
      copyCtor(static_cast<u8*>(dst.get(cid).get()), static_cast<u8*>(src.get(cid).get()));
  }
}

const ComponentSet& Entity::set() const { return *m_world->getEntitySet(m_id); }

RAMPAGE_END