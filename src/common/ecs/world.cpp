#include "ecs.hpp"
#include "entity.hpp"

RAMPAGE_START

EntityWorld::SetIterator::SetIterator(EntityWorld& world, const ComponentSet* match) :
    m_world(&world), m_base(match), m_next(Set<const ComponentSet*>::const_iterator()) {}

void EntityWorld::SetIterator::reset() { m_next = Set<const ComponentSet*>::const_iterator(); }

bool EntityWorld::SetIterator::hasNext() const {
  Set<const ComponentSet*>::const_iterator next = m_next;
  if (next == Set<const ComponentSet*>::const_iterator())
    next = m_world->m_superSets[m_base].begin();
  else
    next++;

  while (next != m_world->m_superSets[m_base].end() && (m_world->m_sets[*next].empty())) {
    next++;
  }

  return next != m_world->m_superSets[m_base].end();
}

EntityWorld::EntityList& EntityWorld::SetIterator::next() {
  if (m_next == Set<const ComponentSet*>::const_iterator())
    m_next = m_world->m_superSets[m_base].begin();
  else
    m_next++;

  while ((m_world->m_sets[*m_next].empty())) {
    m_next++;
  }

  return m_world->m_sets.find(*m_next)->second;
}

EntityWorld& EntityWorld::SetIterator::getWorld() { return *m_world; }

EntityWorld::EntityIterator::EntityIterator(EntityWorld::SetIterator setIt) : m_setIt(setIt) { reset(); }

void EntityWorld::EntityIterator::reset() {
  m_setIt.reset();
  m_next = EntityListIterator();
  m_list = nullptr;
}

bool EntityWorld::EntityIterator::hasNext() const {
  EntityListIterator next = m_next;

  // If you are getting an error from here, make sure beginDefer() and
  // endDefer() are being used.
  return (m_list != nullptr && ++next != m_list->end()) || m_setIt.hasNext();
}

Entity EntityWorld::EntityIterator::next() {
  if (m_list == nullptr || ++m_next == m_list->end()) {
    m_list = &m_setIt.next();
    m_next = m_list->begin();
    return m_setIt.getWorld().get(*m_next);
  }

  return m_setIt.getWorld().get(*m_next);
}

EntityWorld& EntityWorld::EntityIterator::getWorld() { return m_setIt.getWorld(); }

EntityWorld::EntityWorld(IHost& host)
  : m_host(host) {
  m_sets.reserve(10000);
  m_sets.insert(std::make_pair(findOrCreateSet(ComponentSet()), EntityList()));

  component<Destroy>();
  component<Enabled>();
}

EntityWorld::~EntityWorld() {
  for (int i = 1; i < m_componentPools.size(); i++) {
    delete m_componentPools[i];
  }

  for (int i = m_contextsLifo.size() - 1; i >= 0; i--) {
    ContextData& data = m_contexts[m_contextsLifo[i]];

    data.destroy(data.bytes);
  }
}

IHost& EntityWorld::getHost() {
  return m_host;
}

void EntityWorld::observe(EventType event, ComponentId comp, const ComponentSet& with,
                          ObserverCallback callback) {
  auto forwardLambda = [callback](Entity entity, float deltaTime) { callback(entity); };

  m_observers[event][comp].emplace_back(ObserverData(findOrCreateSet(with), callback));
}

Entity EntityWorld::create(EntityId explicitId) {
  EntityId id;
  if (explicitId == NullEntityId) {
    id = m_idMgr.generate();
  } else {
    if (exists(explicitId))
      return Entity(*this, explicitId);

    id = explicitId;
    m_idMgr.ensure(id);
  }

  const ComponentSet* enabledSet = findOrCreateSet(set<Enabled>());
  EntityData data;
  data.comps = enabledSet;
  data.pos = m_sets[enabledSet].emplace(m_sets[enabledSet].end(), id);

  if (id >= m_entities.size())
    m_entities.resize(id + 1);
  m_entities[id].emplace(data);

  return Entity(*this, id);
}

Entity EntityWorld::get(EntityId id) {
  assert(exists(id));
  return Entity(*this, id);
}

Entity EntityWorld::ensure(EntityId id) {
  if (exists(id))
    return get(id);

  return create(id);
}

bool EntityWorld::exists(EntityId id) { return m_idMgr.exists(id); }

bool EntityWorld::isAlive(EntityId id) { return exists(id) && !Entity(*this, id).has<Destroy>(); }

EntityWorld::EntityListIterator EntityWorld::destroy(EntityId id) {
  assert(exists(id));

  Entity entity = get(id);
  EntityData& data = m_entities[id].value();
  entity.add(set<Destroy>());

  if (m_isDefer) {
    m_deferredDestroy.push_back(id);
    return EntityWorld::EntityListIterator();
  }

  // We remove all other components first, to ensure
  // that observers may know that the entity they are processing is currently
  // being destroyed.
  entity.remove(entity.set().remove(component<Destroy>()));

  EntityListIterator next = m_sets[findOrCreateSet({component<Destroy>()})].erase(data.pos);
  m_entities[id].reset();
  m_idMgr.destroy(id);

  return next;
}

void EntityWorld::enable(EntityId entity) { Entity(*this, entity).enable(); }

void EntityWorld::disable(EntityId entity) { Entity(*this, entity).disable(); }

void EntityWorld::removeAll(const ComponentSet& components, bool notify) {
  EntityIterator it = getWithDisabled(components);

  beginDefer();
  while (it.hasNext()) {
    it.next().remove(components, notify);
  }
  endDefer();
}

void EntityWorld::destroyAllEntitiesWith(const ComponentSet& components, bool notify) {
  EntityIterator it = getWithDisabled(components);

  beginDefer();
  while (it.hasNext()) {
    destroy(it.next());
  }
  endDefer();
}

EntityWorld::EntityIterator EntityWorld::getWith(const ComponentSet& components) {
  return getWithDisabled(components.add(component<Enabled>()));
}

EntityWorld::EntityIterator EntityWorld::getWithDisabled(const ComponentSet& components) {
  return EntityIterator(SetIterator(*this, findOrCreateSet(components)));
}

Entity EntityWorld::getFirstWith(const ComponentSet& components) {
  EntityIterator it = getWith(components);
  while (it.hasNext())
    return it.next();

  return Entity(*this, 0);
}

struct EntityChanged {
  ComponentSetBuilder removed, added;
};

System EntityWorld::system(const ComponentSet& set, const SystemFunc& func) {
  return systemWithDisabled(set.add(component<Enabled>()), func);
}

System EntityWorld::systemWithDisabled(const ComponentSet& components, const SystemFunc& func) {
  return System(getWithDisabled(components), func);
}

void EntityWorld::setLocalRange(EntityId startingId) { m_idMgr.setLocalRange(startingId); }

void EntityWorld::enableRangeCheck(bool enable) { m_idMgr.enableRangeCheck(enable); }

bool EntityWorld::validRange(EntityId id) { return m_idMgr.validRange(id); }

void EntityWorld::beginDefer() {
  assert(!m_isDefer);
  assert(m_deferredMoves.empty());
  assert(m_deferredDestroy.empty());
  assert(m_deferredSuperSetCalc.empty());

  m_isDefer = true;
}

void EntityWorld::endDefer() {
  assert(m_isDefer);

  m_isDefer = false;
  for (const ComponentSet* baseSet : m_deferredSuperSetCalc) {
    addToSuperSets(baseSet);
  }

  for (auto const& [id, set] : m_deferredMoves) {
    moveSets(id, set);
  }

  for (EntityId id : m_deferredDestroy) {
    destroy(id);
  }

  m_deferredSuperSetCalc.clear();
  m_deferredMoves.clear();
  m_deferredDestroy.clear();
}

bool EntityWorld::isDefer() { return m_isDefer; }

Entity EntityWorld::clone(EntityId entity) {
  const ComponentSet& compSet = *getEntitySet(entity);

  Entity cloned = create();
  cloned.add(compSet);

  for (ComponentId cid : compSet.list()) {
    auto copyCtor = m_componentCopyCtor[cid];

    if (copyCtor)
      copyCtor(static_cast<u8*>(cloned.get(cid).get()), static_cast<u8*>(get(entity).get(cid).get()));
  }

  return cloned;
}

void EntityWorld::moveSets(EntityId id, const ComponentSet* to) {
  EntityData& entity = m_entities[id].value();

  m_sets.at(entity.comps).erase(entity.pos);
  entity.comps = to;
  entity.pos = m_sets.at(to).emplace(m_sets.at(to).end(), id);
  ; // ALWAYS INSERT IN THE BACK!
}

void EntityWorld::tryMoveSets(EntityId id, const ComponentSet* to) {
  if (getEntitySet(id) == to)
    return;

  if (!m_isDefer) {
    return moveSets(id, to);
  } else {
    m_deferredMoves[id] = to;
  }
}

const ComponentSet* EntityWorld::findOrCreateSet(const ComponentSet& base) {
  if (m_componentSets.find(base) == m_componentSets.end()) {
#ifndef NDEBUG
    for (ComponentId id : base.list())
      assert(id < m_componentPools.size());
#endif

    m_componentSets.insert(std::make_pair(base, new ComponentSet(base)));

    const ComponentSet* baseSet = m_componentSets[base];
    m_sets.insert(std::make_pair(baseSet, EntityList()));

    if (m_isDefer) {
      m_deferredSuperSetCalc.push_back(baseSet);
    } else {
      addToSuperSets(baseSet);
    }

    return baseSet;
  } else {
    return m_componentSets.find(base)->second;
  }
}

void EntityWorld::notify(EntityWorld::EventType type, EntityId id, ComponentId compId) {
  Map<ComponentId, std::vector<ObserverData>>& m_typeObservers = m_observers[type];

  if (m_typeObservers.find(compId) == m_typeObservers.end())
    return;

  for (EntityWorld::ObserverData& obvData : m_typeObservers.at(compId)) {
    if (getEntitySet(id)->superset(*obvData.with)) {
      obvData.callback(get(id));
    }
  }
}

bool EntityWorld::hasComponent(EntityId id, ComponentId comp) { return getEntitySet(id)->has(comp); }

const ComponentSet* EntityWorld::getEntitySet(EntityId id) {
  Map<EntityId, const ComponentSet*>::iterator it = m_deferredMoves.find(id);
  if (it == m_deferredMoves.end()) {
    return m_entities[id].value().comps;
  }

  return it->second;
}

void EntityWorld::addToSuperSets(const ComponentSet* baseSet) {
  assert(!m_isDefer);

  m_superSets[baseSet].insert(baseSet);

  std::vector<const ComponentSet*> supersets; // the supersets of baseSet
  std::vector<const ComponentSet*> subsets; // the subSets of baseSet
  /* Find all sets that contain this new set and insert it into m_superSets */
  for (const std::pair<const ComponentSet* const, Set<const ComponentSet*>>& set : m_superSets)
    if (set.first == baseSet)
      continue;
    else if (set.first->superset(*baseSet))
      supersets.push_back(set.first);
    else if (baseSet->superset(*set.first))
      subsets.push_back(set.first);

  for (const ComponentSet* set : supersets)
    m_superSets[baseSet].insert(set);
  for (const ComponentSet* set : subsets)
    m_superSets[set].insert(baseSet);
}

RAMPAGE_END