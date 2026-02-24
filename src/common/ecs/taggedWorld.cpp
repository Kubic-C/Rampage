#include "taggedWorld.hpp"
#include "entityPtr.hpp"

RAMPAGE_START

TaggedEntityWorld::TaggedEntityWorld(IWorldPtr realWorld, ComponentId worldTagComponentId, PrivateConstructorTag)
  : m_realWorld(realWorld), m_worldTagComponentId(worldTagComponentId) {}

IHost& TaggedEntityWorld::getHost() {
  return m_realWorld->getHost();
}

void TaggedEntityWorld::addContext(ContextId id, u8* bytes, std::function<void(u8*)> destroy) noexcept {
  m_realWorld->addContext(id, bytes, destroy);
}

u8* TaggedEntityWorld::getContext(ContextId id) {
  return m_realWorld->getContext(id);
}

EntityPtr TaggedEntityWorld::create(EntityId explicitId) {
  EntityId eid = m_realWorld->create(explicitId);
  m_realWorld->add(eid, ComponentSet{m_worldTagComponentId});
  return EntityPtr(m_self, eid);
}

EntityPtr TaggedEntityWorld::getEntity(EntityId id) {
  return EntityPtr(m_self, id);
}

EntityPtr TaggedEntityWorld::ensure(EntityId eid) {
  m_realWorld->ensure(eid);
  if (!m_realWorld->has(eid, m_worldTagComponentId)) {
    m_realWorld->add(eid, ComponentSet{m_worldTagComponentId});
  }
  return EntityPtr(m_self, eid);
}

bool TaggedEntityWorld::exists(EntityId id) {
  return m_realWorld->exists(id);
}

bool TaggedEntityWorld::isAlive(EntityId id) {
  return m_realWorld->isAlive(id);
}

void TaggedEntityWorld::destroy(EntityId id) {
  m_realWorld->destroy(id);
}

void TaggedEntityWorld::enable(EntityId entity) {
  m_realWorld->enable(entity);
}

void TaggedEntityWorld::disable(EntityId entity) {
  m_realWorld->disable(entity);
}

EntityPtr TaggedEntityWorld::clone(EntityId entity) {
  EntityId clonedId = m_realWorld->clone(entity).id();
  // Tag is automatically copied during clone, but ensure it's present
  if (!m_realWorld->has(clonedId, m_worldTagComponentId)) {
    m_realWorld->add(clonedId, ComponentSet{m_worldTagComponentId});
  }
  return EntityPtr(m_self, clonedId);;
}

size_t TaggedEntityWorld::getEntityCount() const {
  return m_realWorld->getEntityCount();
}

size_t TaggedEntityWorld::getSetCount() const {
  return m_realWorld->getSetCount();
}

void TaggedEntityWorld::removeAll(const ComponentSet& components, bool notify) {
  m_realWorld->removeAll(components, notify);
}

void TaggedEntityWorld::destroyAllEntitiesWith(const ComponentSet& components, bool notify) {
  m_realWorld->destroyAllEntitiesWith(components, notify);
}

IEntityIteratorPtr TaggedEntityWorld::getWith(const ComponentSet& components) {
  return m_realWorld->getWith(components);
}

IEntityIteratorPtr TaggedEntityWorld::getWithDisabled(const ComponentSet& components) {
  return m_realWorld->getWithDisabled(components);
}

EntityPtr TaggedEntityWorld::getFirstWith(const ComponentSet& components) {
  EntityPtr entity = m_realWorld->getFirstWith(components);
  if (!entity.isNull()) {
    return EntityPtr(m_self, entity.id());
  }
  return entity;
}

void TaggedEntityWorld::setLocalRange(EntityId startingId) {
  m_realWorld->setLocalRange(startingId);
}

void TaggedEntityWorld::enableRangeCheck(bool enable) {
  m_realWorld->enableRangeCheck(enable);
}

bool TaggedEntityWorld::validRange(EntityId id) {
  return m_realWorld->validRange(id);
}

void TaggedEntityWorld::beginDefer() {
  m_realWorld->beginDefer();
}

void TaggedEntityWorld::endDefer() {
  m_realWorld->endDefer();
}

bool TaggedEntityWorld::isDefer() {
  return m_realWorld->isDefer();
}

Ref TaggedEntityWorld::get(EntityId entity, ComponentId compId) {
  return m_realWorld->get(entity, compId);
}

void TaggedEntityWorld::add(EntityId entity, const ComponentSet& addComps, bool emit) {
  m_realWorld->add(entity, addComps, emit);
}

void TaggedEntityWorld::remove(EntityId entity, const ComponentSet& remComps, bool emit) {
  // Prevent removal of the world tag component
  ComponentSet filtered = remComps.remove(m_worldTagComponentId);
  m_realWorld->remove(entity, filtered, emit);
}

bool TaggedEntityWorld::has(EntityId entity, ComponentId compId) {
  return m_realWorld->has(entity, compId);
}

void TaggedEntityWorld::copy(EntityId src, EntityId dst, const ComponentSet& comps) {
  m_realWorld->copy(src, dst, comps);
}

void TaggedEntityWorld::move(EntityId src, EntityId dst, const ComponentSet& comps) {
  m_realWorld->move(src, dst, comps);
}

const ComponentSet& TaggedEntityWorld::setOf(EntityId entity) {
  return m_realWorld->setOf(entity);
}

IPool* TaggedEntityWorld::getPool(ComponentId id) {
  return m_realWorld->getPool(id);
}

ComponentId TaggedEntityWorld::component(ComponentId compId, bool isRegistered, const std::string& name,
  size_t size, NewPoolFunc newPoolFunc, ComponentCopyCtor copyCtor, ComponentMoveCtor moveCtor,
  SerializeFunc serializeFunc, DeserializeFunc deserializeFunc) noexcept {
  return m_realWorld->component(compId, isRegistered, name, size, newPoolFunc, copyCtor, moveCtor, serializeFunc, deserializeFunc);
}

void TaggedEntityWorld::observe(ComponentId eventType, ComponentId comp, const ComponentSet& with, ObserverCallback callback) {
  m_realWorld->observe(eventType, comp, with, callback);
}

void TaggedEntityWorld::emit(ComponentId eventType, EntityId entity, ComponentId comp) {
  m_realWorld->emit(eventType, entity, comp);
}

RAMPAGE_END