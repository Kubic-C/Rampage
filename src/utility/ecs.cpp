#include "ecs.hpp"

ComponentSet::ComponentSet(const std::vector<ComponentId>& ids)
  : m_comps(ids) {
  std::sort(m_comps.begin(), m_comps.end());
}

ComponentSet::ComponentSet(const std::initializer_list<ComponentId>& ids)
  : m_comps(ids) {
  std::sort(m_comps.begin(), m_comps.end());
}

const ComponentSet ComponentSet::add(ComponentId id) const {
  ComponentSet addSet = *this;

  auto it = std::lower_bound(addSet.m_comps.begin(), addSet.m_comps.end(), id);
  if (it == addSet.m_comps.end() || *it != id)
    addSet.m_comps.insert(it, id);

  return addSet;
}

const ComponentSet ComponentSet::remove(ComponentId id) const {
  ComponentSet removeSet = *this;

  auto it = std::lower_bound(removeSet.m_comps.begin(), removeSet.m_comps.end(), id);
  if (it != removeSet.m_comps.end() && *it == id)
    removeSet.m_comps.erase(it);

  return removeSet;
}

bool ComponentSet::has(ComponentId id) const {
  if (m_comps.empty())
    return false;

  auto it = std::lower_bound(m_comps.begin(), m_comps.end(), id);
  if (it != m_comps.end() && *it == id)
    return true;
  else
    return false;
}

bool ComponentSet::superset(const ComponentSet& set) const {
  for (ComponentId otherId : set.m_comps)
    if (!has(otherId))
      return false;

  return true;
}

bool ComponentSet::subset(const ComponentSet& set) const {
  for (ComponentId thisId : m_comps)
    if (!set.has(thisId))
      return false;

  return true;
}

ComponentSetId ComponentSet::getSetId() const {
  ComponentSetId hashValue = 0;
  for (const auto& el : m_comps) {
    hashValue ^= std::hash<ComponentId>()(el) + 0x9e3779b9 + (hashValue << 6) + (hashValue >> 2);
  }
  return hashValue;
}

const std::vector<ComponentId>& ComponentSet::list() const {
  return m_comps;
}

ComponentSetBuilder::ComponentSetBuilder(const ComponentSet& ids)
  : m_comps(ids.list()) {}

ComponentSetBuilder::ComponentSetBuilder(const std::initializer_list<ComponentId>& ids)
  : m_comps(ids) {
  std::sort(m_comps.begin(), m_comps.end());
}

const void ComponentSetBuilder::add(ComponentId id) {
  auto it = std::lower_bound(m_comps.begin(), m_comps.end(), id);
  if (it == m_comps.end() || *it != id)
    m_comps.insert(it, id);

}

const void ComponentSetBuilder::add(const ComponentSet& set) {
  m_comps.reserve(set.list().size());
  for (ComponentId id : set.list())
    add(id);
}

const void ComponentSetBuilder::remove(ComponentId id) {
  auto it = std::lower_bound(m_comps.begin(), m_comps.end(), id);
  if (it != m_comps.end() && *it == id)
    m_comps.erase(it);
}

const void ComponentSetBuilder::remove(const ComponentSet& set) {
  for (ComponentId id : set.list())
    remove(id);
}

bool ComponentSetBuilder::has(ComponentId id) const {
  if (m_comps.empty())
    return false;

  auto it = std::lower_bound(m_comps.begin(), m_comps.end(), id);
  if (it != m_comps.end() && *it == id)
    return true;
  else
    return false;
}

bool ComponentSetBuilder::superset(const ComponentSet& set) const {
  for (ComponentId otherId : set.m_comps)
    if (!has(otherId))
      return false;

  return true;
}

bool ComponentSetBuilder::subset(const ComponentSet& set) const {
  for (ComponentId thisId : m_comps)
    if (!set.has(thisId))
      return false;

  return true;
}

ComponentSetId ComponentSetBuilder::getSetId() const {
  ComponentSetId hashValue = 0;
  for (const auto& el : m_comps) {
    hashValue ^= std::hash<ComponentId>()(el) + 0x9e3779b9 + (hashValue << 6) + (hashValue >> 2);
  }
  return hashValue;
}

const std::vector<ComponentId>& ComponentSetBuilder::list() const {
  return m_comps;
}

ComponentSet ComponentSetBuilder::build() const {
  return ComponentSet(m_comps);
}

EntityWorld::SetIterator::SetIterator(EntityWorld& world, const ComponentSet* match)
  : m_world(world), m_base(match), m_next(Set<const ComponentSet*>::const_iterator()) {
}

void EntityWorld::SetIterator::reset() {
  m_next = Set<const ComponentSet*>::const_iterator();
}

bool EntityWorld::SetIterator::hasNext() const {
  Set<const ComponentSet*>::const_iterator next = m_next;
  if (next == Set<const ComponentSet*>::const_iterator())
    next = m_world.m_superSets[m_base].begin();
  else
    next++;

  while(next != m_world.m_superSets[m_base].end() && (m_world.m_sets[*next].empty())) {
    next++;
  } 

  return next != m_world.m_superSets[m_base].end();
} 

EntityWorld::EntityList& EntityWorld::SetIterator::next() {
  if (m_next == Set<const ComponentSet*>::const_iterator())
    m_next = m_world.m_superSets[m_base].begin();
  else
    m_next++;

  while ((m_world.m_sets[*m_next].empty())) {
    m_next++;
  }

  return m_world.m_sets.find(*m_next)->second;
}

EntityWorld& EntityWorld::SetIterator::getWorld() {
  return m_world;
}

EntityWorld::EntityIterator::EntityIterator(EntityWorld::SetIterator setIt)
  : m_setIt(setIt) {
  reset();
}

void EntityWorld::EntityIterator::reset() {
  m_setIt.reset();
  m_next = EntityListIterator();
  m_list = nullptr;
}

bool EntityWorld::EntityIterator::hasNext() const {
  EntityListIterator next = m_next;
  
  // If you are getting an error from here, make sure beginDefer() and endDefer() are being used.
  return (m_list != nullptr &&
      ++next != m_list->end()) ||
    m_setIt.hasNext();
}

Entity EntityWorld::EntityIterator::next() {
  if (m_list == nullptr || ++m_next == m_list->end()) {
    m_list = &m_setIt.next();
    m_next = m_list->begin();
    return m_setIt.getWorld().get(*m_next);
  }
  
  return m_setIt.getWorld().get(*m_next);
}

EntityWorld& EntityWorld::EntityIterator::getWorld() {
  return m_setIt.getWorld();
}

EntityWorld::EntityWorld() {
  m_sets.reserve(10000);
  m_sets.insert(std::make_pair(findOrCreateSet(ComponentSet()), EntityList()));

  component<Destroy>();
  component<ModuleData>();
  component<Enabled>();
}

EntityWorld::~EntityWorld() {
  for (int i = m_contextsLifo.size() - 1; i >= 0; i--) {
    ContextData& data = m_contexts[m_contextsLifo[i]];

    data.destroy(data.bytes);
  }
}

void EntityWorld::observe(EventType event, ComponentId comp, const ComponentSet& with, ObserverCallback callback) {
  auto forwardLambda =
    [callback](Entity entity, float deltaTime) {
      callback(entity); 
    };

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

  m_entities.insert(std::make_pair(id, data));

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

bool EntityWorld::exists(EntityId id) {
  return m_entities.find(id) != m_entities.end();
}

bool EntityWorld::isAlive(EntityId id) {
  return exists(id) && !Entity(*this, id).has<Destroy>();
}

EntityWorld::EntityListIterator EntityWorld::destroy(EntityId id) {
  assert(exists(id));

  Entity entity = get(id);
  EntityData& data = m_entities.find(id)->second;
  entity.add(set<Destroy>());

  if (m_isDefer) {
    m_deferredDestroy.push_back(id);
    return EntityWorld::EntityListIterator();
  }

  // We remove all other components first, to ensure
  // that observers may know that the entity they are processing is currently being destroyed.
  entity.remove(entity.set().remove(component<Destroy>()));

  EntityListIterator next = m_sets[findOrCreateSet({ component<Destroy>() })].erase(data.pos);
  m_entities.erase(id);
  m_idMgr.destroy(id);
     
  return next;
}

void EntityWorld::enable(EntityId entity) {
  Entity(*this, entity).enable();
}

void EntityWorld::disable(EntityId entity) {
  Entity(*this, entity).disable();
}

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

void EntityWorld::run(float deltaTime) {
  EntityIterator it = getWith(set<ModuleData>());
  while (it.hasNext()) {
    Entity e = it.next();
    ModuleData& data = e.get<ModuleData>();

    data.m_module->run(*this, deltaTime);
  }
}

System EntityWorld::system(const ComponentSet& set, const SystemFunc& func) {
  return systemWithDisabled(set.add(component<Enabled>()), func);
}

System EntityWorld::systemWithDisabled(const ComponentSet& components, const SystemFunc& func) {
  return System(getWithDisabled(components), func);
}

void EntityWorld::setLocalRange(EntityId startingId) {
  m_idMgr.setLocalRange(startingId);
}

void EntityWorld::enableRangeCheck(bool enable) {
  m_idMgr.enableRangeCheck(enable);
}

bool EntityWorld::validRange(EntityId id) {
  return m_idMgr.validRange(id);
}

void EntityWorld::beginDefer() {
  assert(!m_isDefer);
  assert(m_deferredMoves.empty());
  assert(m_deferredDestroy.empty());

  m_isDefer = true;
}

void EntityWorld::endDefer() {
  assert(m_isDefer);

  m_isDefer = false;
  for (auto const& [id, set] : m_deferredMoves) {
    moveSets(id, set);
  }

  for (EntityId id: m_deferredDestroy) {
    destroy(id);
  }

  m_deferredMoves.clear();
  m_deferredDestroy.clear();
}

bool EntityWorld::isDefer() {
  return m_isDefer;
}

Entity EntityWorld::clone(EntityId entity) {
  const ComponentSet& compSet = *getEntitySet(entity);

  Entity cloned = create();
  cloned.add(compSet);

  for (ComponentId cid : compSet.list()) {
    ComponentData& compData = m_components.find(cid)->second;

    if(compData.m_copyCtor)
      compData.m_copyCtor(cloned.get(cid), get(entity).get(cid));
  }

  return cloned;
}

EntityWorld::ComponentData& EntityWorld::getComponentData(ComponentId id) {
  return m_components.at(id);
}

void EntityWorld::moveSets(EntityId id, const ComponentSet* to) {
  EntityData& entity = m_entities.find(id)->second;

  m_sets.at(m_entities.at(id).comps).erase(entity.pos);
  entity.comps = to;
  entity.pos = m_sets.at(to).emplace(m_sets.at(to).end(), id);; // ALWAYS INSERT IN THE BACK!
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
      assert(m_components.contains(id));
#endif

    m_componentSets.insert(std::make_pair(base, new ComponentSet(base)));

    const ComponentSet* baseSet = m_componentSets[base];
    m_sets.insert(std::make_pair(baseSet, EntityList()));
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
    for(const ComponentSet* set : subsets)
      m_superSets[set].insert(baseSet);

    return baseSet;
  }
  else {
    return m_componentSets.find(base)->second;
  }
}

void EntityWorld::notify(EntityWorld::EventType type, EntityId id, ComponentId compId) {
  Map<ComponentId, std::vector<ObserverData>>& m_typeObservers = m_observers[type];

  if (m_typeObservers.find(compId) == m_typeObservers.end())
    return;

  for (EntityWorld::ObserverData& obvData : m_typeObservers.at(compId)) {
    if (getEntitySet(id)->superset(*obvData.with))
      obvData.callback(get(id));
  }
}

bool EntityWorld::hasComponent(EntityId id, ComponentId comp) {
  Map<EntityId, const ComponentSet*>::iterator it = m_deferredMoves.find(id);
  if (it == m_deferredMoves.end()) {
    return m_entities.find(id)->second.comps->has(comp);
  }

  return it->second->has(comp);
}

const ComponentSet* EntityWorld::getEntitySet(EntityId id) {
  Map<EntityId, const ComponentSet*>::iterator it = m_deferredMoves.find(id);
  if (it == m_deferredMoves.end()) {
    return m_entities.find(id)->second.comps;
  }

  return it->second;
}

Entity::Entity(EntityWorld& world)
  : m_world(world), m_id(NullEntityId) {}

Entity::Entity(EntityWorld& world, EntityId id)
  : m_world(world), m_id(id) {
}

Entity& Entity::operator=(Entity other) {
  m_world = other.m_world;
  m_id = other.m_id;

  return *this;
}

EntityId Entity::id() const {
  return m_id;
}

bool Entity::exists() const {
  return m_world.exists(m_id);
}

bool Entity::alive() const {
  return m_world.isAlive(m_id);
}

bool Entity::isNull() const {
  return m_id == NullEntityId;
}

void Entity::add(ComponentId compId, bool notify) {
  add(ComponentSet({ compId }), notify);
}

void Entity::remove(ComponentId compId, bool notify) {
  remove(ComponentSet({ compId }), notify);
}

void Entity::add(const ComponentSet& addComps, bool notify) {
  const ComponentSet& oldSet = set();

  ComponentSetBuilder tempSet(oldSet);
  for (ComponentId compId : addComps.list()) {
    if (oldSet.has(compId))
      continue;

    EntityWorld::ComponentData& compData = m_world.getComponentData(compId);
    tempSet.add(compId);
    if (compData.pool)
     compData.pool->create(m_id);
  }
  m_world.tryMoveSets(m_id, m_world.findOrCreateSet(tempSet.build()));

  if (notify && &oldSet != m_world.findOrCreateSet(tempSet.build()))
    for (ComponentId compId : addComps.list()) {
      if (oldSet.has(compId))
        continue;
      m_world.notify(EntityWorld::EventType::Add, m_id, compId);
    }
}

void Entity::remove(const ComponentSet& remComps, bool notify ) {
#ifndef NDEBUG
  {
    const ComponentSet& oldSet = set();
    for (ComponentId compId : remComps.list())
      assert(oldSet.has(compId));
  }
#endif

  if (notify)
    for (ComponentId compId : remComps.list())
        m_world.notify(EntityWorld::EventType::Remove, m_id, compId);

  const ComponentSet& oldSet = set();
  ComponentSetBuilder tempSet(oldSet);
  for (ComponentId compId : remComps.list()) {
    EntityWorld::ComponentData& compData = m_world.getComponentData(compId);
    if (!oldSet.has(compId))
      throw std::exception("Set does not contain compId: " + compId);

    tempSet.remove(compId);
    if (compData.pool)
      compData.pool->destroy(m_id);
  }

   m_world.tryMoveSets(m_id, m_world.findOrCreateSet(tempSet.build()));
}

u8* Entity::get(ComponentId compId) {
  assert(has(compId));

  EntityWorld::ComponentData& compData = m_world.getComponentData(compId);
  return compData.pool->get(m_id);
}

bool Entity::has(ComponentId compId) {
  return m_world.hasComponent(m_id, compId);
}

EntityWorld& Entity::world() {
  return m_world;
}

void Entity::enable() {
  add<EntityWorld::Enabled>();
}

void Entity::disable() {
  remove<EntityWorld::Enabled>();
}

Entity Entity::clone() {
  return m_world.clone(m_id);
}

const ComponentSet& Entity::set() const {
  return *m_world.getEntitySet(m_id);
}