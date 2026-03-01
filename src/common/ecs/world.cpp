#include "world.hpp"
#include "ecs.hpp"
#include "entityPtr.hpp"
#include "../ihost.hpp"
#include "taggedWorld.hpp"

#include <fstream>
#include <capnp/serialize-packed.h>
#include <capnp/compat/json.h>

RAMPAGE_START

class AssetLoaderImpl : public IAssetLoaderImpl {
  using JSONTypeHandlerFunc = std::function<bool(IWorldPtr world, const std::string_view& name, const json& json)>;
public:
  AssetLoaderImpl() {
    m_jsonHandlers["Entity"] = 
    [&](IWorldPtr world, const std::string_view& name, const json& entityJson) {
      EntityPtr entity = world->getAssetLoader().createAsset(std::string(name));

      if(!entityJson.contains("components"))
        return false;
      json componentList = entityJson["components"];
      if(!componentList.is_array())
        return false;

      for(json componentJson : componentList) {
        if(!componentJson.is_object())
          return false;
        if(!componentJson.contains("type") || !componentJson["type"].is_string())
          return false;
        std::string type = componentJson["type"];
        if(!m_componentJsonHandlers.contains(type))
          return false;
        ComponentId compId = m_componentJsonHandlers[type].compId;

        entity.add(compId);
        m_componentJsonHandlers[type].fromJsonFunc(entity.get(compId), AssetLoader(world, *this), componentJson);
      }

      return true;
    };
  }

  virtual ~AssetLoaderImpl() = default;

  virtual EntityPtr createAsset(IWorldPtr world, const std::string& name) override {
    EntityPtr entity = world->create();
    AssetId assetId = m_assetIdManager.generate();
    m_assetToEntity[assetId] = entity.id();
    m_assetsByName[name] = assetId;
    entity.disable();
    return entity;
  }

  bool loadAssetsFromFile(IWorldPtr world, const char* path) override {
    std::ifstream file(path);
    if (!file.is_open()) {
      world->getHost().log("<bgRed>Failed to open asset file: %s<reset>", path);
      return false;
    }
    std::string jsonStr = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    return loadAssetsFromString(world, jsonStr);
  }

  bool loadAssetsFromString(IWorldPtr _world, const std::string& string) override {
    IWorldPtr world = TaggedEntityWorld::create(_world, _world->component<AssetTag>());    

    json assetList = json::parse(string, nullptr, false, true);
    if (assetList.is_discarded()) {
      world->getHost().log("<bgRed>Failed to parse JSON from asset string: %s<reset>", string.c_str());
      return false;
    }
    if(!assetList.is_array()) {
      world->getHost().log("<bgRed>Expected JSON array in asset string: %s<reset>", string.c_str());
      return false;
    }

    for(json object : assetList) {
      if(!object.is_object()) {
        world->getHost().log("<bgRed>Expected JSON object in asset array: %s<reset>", string.c_str());
        continue;
      }
      if(!object.contains("name") || !object["name"].is_string()) {
        world->getHost().log("<bgRed>Asset missing 'name' field or 'name' is not a string: %s<reset>", string.c_str());
        continue;
      }
      std::string name = object["name"];
      if(m_assetsByName.contains(name)) {
        world->getHost().log("<bgRed>Duplicate asset name '%s' in asset string: %s<reset>", name.c_str(), string.c_str());
        continue;
      }
      json data = object["data"];
      std::string type = data["type"];
      if(!m_jsonHandlers.contains(type)) {
        world->getHost().log("<bgRed>No JSON handler registered for type '%s' in asset string: %s<reset>", type.c_str(), string.c_str());
        continue;
      }

      if(!m_jsonHandlers[type](world, std::string(name), data)) {
        world->getHost().log("<bgRed>JSON handler for type '%s' failed to load asset '%s' in asset string: %s<reset>", type.c_str(), name.c_str(), string.c_str());
        continue;
      }
    }

    return true;
  }

  AssetId getAssetIdByName(const std::string& name) override {
    auto it = m_assetsByName.find(name);
    if (it == m_assetsByName.end()) {
      // TODO: Handle not found - throw or return invalid?
      return AssetId(0);  // Invalid
    }
    return it->second;
  }

  EntityId getEntityIdByAssetId(AssetId assetId) override {
    auto it = m_assetToEntity.find(assetId);
    if (it == m_assetToEntity.end()) {
      // TODO: Handle not found
      return NullEntityId;
    }
    return it->second;
  }
  
  void registerComponent(IWorldPtr world, ComponentId compId, FromJsonFunc fromJsonFunc) override {
    m_componentJsonHandlers[std::string(world->nameOf(compId))] = { compId, fromJsonFunc };
  }

private:
  OpenMap<AssetId, EntityId> m_assetToEntity;
  OpenMap<std::string, AssetId> m_assetsByName;
  IdManager<AssetId> m_assetIdManager;

  struct ComponentJsonHandler {
    ComponentId compId;
    FromJsonFunc fromJsonFunc;
  };

  OpenMap<std::string, ComponentJsonHandler> m_componentJsonHandlers;
  OpenMap<std::string, JSONTypeHandlerFunc> m_jsonHandlers;
};

class SetIterator {
public:
  SetIterator& operator=(const SetIterator& other) {
    m_base = other.m_base;
    m_next = other.m_next;
    m_world = other.m_world;

    return *this;
  }

  SetIterator(EntityWorld& world, const ComponentSet* match) :
      m_world(&world), m_base(match), m_next(Set<const ComponentSet*>::const_iterator()) {}

  void reset() {
    m_next = Set<const ComponentSet*>::const_iterator();
  }

  bool hasNext() const {
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

  EntityWorld::EntityList& next() {
    if (m_next == Set<const ComponentSet*>::const_iterator())
      m_next = m_world->m_superSets[m_base].begin();
    else
      m_next++;

    while ((m_world->m_sets[*m_next].empty())) {
      m_next++;
    }

    return m_world->m_sets.find(*m_next)->second;
  }

  EntityWorld& getWorld() {
    return *m_world;
  }

private:
  EntityWorld* m_world;
  const ComponentSet* m_base;
  Set<const ComponentSet*>::const_iterator m_next;
};

class EntityIterator : public IEntityIterator {
public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = EntityPtr;
  using difference_type = std::ptrdiff_t;
  using pointer = EntityPtr*;
  using reference = EntityPtr&;

  EntityIterator& operator=(const EntityIterator& other) {
    m_setIt = other.m_setIt;
    m_next = other.m_next;
    m_list = other.m_list;
    return *this;
  }

  EntityIterator(SetIterator setIt) : m_setIt(setIt) {
    reset();
  }

  void reset() {
    m_setIt.reset();
    m_next = EntityWorld::EntityListIterator();
    m_list = nullptr;
  }

  bool hasNext() const {
    EntityWorld::EntityListIterator next = m_next;

    // If you are getting an error from here, make sure beginDefer() and
    // endDefer() are being used.
    return (m_list != nullptr && ++next != m_list->end()) || m_setIt.hasNext();
  }

  EntityPtr next() {
    if (m_list == nullptr || ++m_next == m_list->end()) {
      m_list = &m_setIt.next();
      m_next = m_list->begin();
      return m_setIt.getWorld().getEntity(*m_next);
    }

    return m_setIt.getWorld().getEntity(*m_next);
  }

  IWorld& getWorld() {
    return m_setIt.getWorld();
  }

private:
  EntityWorld::EntityList* m_list;
  EntityWorld::EntityListIterator m_next;
  SetIterator m_setIt;
};

EntityWorld::EntityWorld(IHost& host, PrivateConstructorTag) : m_host(host), m_assetLoader(std::make_unique<AssetLoaderImpl>()) {
  m_sets.reserve(10000);
  m_sets.insert(std::make_pair(findOrCreateSet(ComponentSet()), EntityList()));

  component<Enabled>(false);
  component<Destroy>(false);
  component<ComponentAddedEvent>(false);
  component<ComponentRemovedEvent>(false);
}

EntityWorld::~EntityWorld() noexcept {
  for (int i = 1; i < m_componentPools.size(); i++) {
    delete m_componentPools[i];
  }

  for (size_t i = m_contextsLifo.size() - 1; i >= 0; i--) {
    ContextData& data = m_contexts[m_contextsLifo[i]];

    data.destroy(data.bytes);
  }
}

IHost& EntityWorld::getHost() {
  return m_host;
}

IWorld& EntityWorld::getTopWorld() {
  return *this;
}

void EntityWorld::addContext(ContextId id, u8* bytes, std::function<void(u8*)> destroy) noexcept {
  if(m_contexts.contains(id))
    throw std::runtime_error("Context already exists");

  m_contexts[id].bytes = bytes;
  m_contexts[id].destroy = destroy;
  m_contextsLifo.push_back(id);
}

u8* EntityWorld::getContext(ContextId id) {
  assert(m_contexts.contains(id));
  return m_contexts[id].bytes;
}

EntityPtr EntityWorld::create(EntityId explicitId) {
  EntityId id;
  if (explicitId == NullEntityId) {
    id = m_idMgr.generate();
  } else {
    if (exists(explicitId))
      return getEntity(explicitId);

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

  return getEntity(id);
}

EntityPtr EntityWorld::getEntity(EntityId id) {
  return EntityPtr(m_self, id);
}

EntityPtr EntityWorld::ensure(EntityId id) {
  if (exists(id))
    return getEntity(id);

  return create(id);
}

bool EntityWorld::exists(EntityId id) {
  return m_idMgr.exists(id);
}

bool EntityWorld::isAlive(EntityId id) {
  return exists(id) && !getEntity(id).has<Destroy>();
}

void EntityWorld::destroy(EntityId id) {
  assert(exists(id));

  EntityData& data = m_entities[id].value();
  add<Destroy>(id);

  if (m_isDefer) {
    m_deferredDestroy.push_back(id);
    return;
  }

  // We remove all other components first, to ensure
  // that observers may know that the entity they are processing is currently
  // being destroyed.
  remove(id, setOf(id).remove(component<Destroy>()));

  EntityListIterator next = m_sets[findOrCreateSet({component<Destroy>()})].erase(data.pos);
  m_entities[id].reset();
  m_idMgr.destroy(id);
}

void EntityWorld::enable(EntityId entity) {
  add<IWorld::Enabled>(entity);
}

void EntityWorld::disable(EntityId entity) {
  remove<IWorld::Enabled>(entity);
}

EntityPtr EntityWorld::clone(EntityId entity) {
  EntityPtr cloned = create();

  copy(entity, cloned);

  return cloned;
}

size_t EntityWorld::getEntityCount() const {
  return m_idMgr.count();
}

size_t EntityWorld::getSetCount() const {
  return m_sets.size();
}

void EntityWorld::removeAll(const ComponentSet& components, bool notify) {
  auto it = getWithDisabled(components);
  
  beginDefer();
  while (it->hasNext()) {
    it->next().remove(components, notify);
  }
  endDefer();
}

void EntityWorld::destroyAllEntitiesWith(const ComponentSet& components, bool notify) {
  auto it = getWithDisabled(components);
  
  beginDefer();
  while (it->hasNext()) {
    destroy(it->next());
  }
  endDefer();
}

IEntityIteratorPtr EntityWorld::getWith(const ComponentSet& components) {
  return getWithDisabled(components.add(component<Enabled>()));
}

IEntityIteratorPtr EntityWorld::getWithDisabled(const ComponentSet& components) {
  return std::make_unique<EntityIterator>(SetIterator(*this, findOrCreateSet(components)));
}

EntityPtr EntityWorld::getFirstWith(const ComponentSet& components) {
  auto it = getWith(components);
  while (it->hasNext())
    return it->next();

  return getEntity(0);
}

struct EntityChanged {
  ComponentSetBuilder removed, added;
};

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

bool EntityWorld::isDefer() {
  return m_isDefer;
}

Ref EntityWorld::get(EntityId entity, ComponentId compId) {
  return Ref(getEntity(entity), compId);
}

std::string_view EntityWorld::nameOf(ComponentId compId) {
  if (compId < m_componentNames.size())
    return m_componentNames[compId];
  return "";
}

void EntityWorld::add(EntityId entity, const ComponentSet& addComps, bool emit) {
  assert(exists(entity));

  const ComponentSet& oldSet = setOf(entity);

  ComponentSetBuilder tempSet(oldSet);
  for (ComponentId compId : addComps.list()) {
    if (oldSet.has(compId))
      continue;

    IPool* pool = getPool(compId);
    tempSet.add(compId);
    if (pool)
      pool->create(entity);
  }
  tryMoveSets(entity, findOrCreateSet(tempSet.build()));

  if (emit && &oldSet != findOrCreateSet(tempSet.build()))
    for (ComponentId compId : addComps.list()) {
      if (oldSet.has(compId))
        continue;
      this->emit(component<ComponentAddedEvent>(), entity, compId);
    }
}

void EntityWorld::remove(EntityId entity, const ComponentSet& remComps, bool emit) {
  assert(exists(entity));

#ifndef NDEBUG
  {
    const ComponentSet& oldSet = setOf(entity);
    for (ComponentId compId : remComps.list())
      assert(oldSet.has(compId));
  }
#endif

  if (emit)
    for (ComponentId compId : remComps.list())
      this->emit(component<ComponentRemovedEvent>(), entity, compId);

  const ComponentSet& oldSet = setOf(entity);
  ComponentSetBuilder tempSet(oldSet);
  for (ComponentId compId : remComps.list()) {
    if (!oldSet.has(compId))
      throw std::runtime_error("Set does not contain compId: " + std::to_string(compId));

    IPool* pool = getPool(compId);
    tempSet.remove(compId);
    if (pool)
      pool->destroy(entity);
  }

  tryMoveSets(entity, findOrCreateSet(tempSet.build()));
}

bool EntityWorld::has(EntityId entity, ComponentId comp) {
  assert(exists(entity));
  return setOf(entity).has(comp);
}

void EntityWorld::copy(EntityId src, EntityId dst, const ComponentSet& copySet) {
  assert(exists(src));
  assert(exists(dst));

  const ComponentSet& entitySet = setOf(src);
  const ComponentSet& comps = copySet.list().empty() ? entitySet : copySet;
  add(dst, comps);

  for (ComponentId cid : comps.list()) {
    auto copyCtor = m_componentCopyCtor[cid];

    // TODO This is not really a full copy, components use
    // their default ctor and then get copy assigned, which may not be what you want.
    if (copyCtor)
      copyCtor(get(src, cid), get(dst, cid));
  }
}

void EntityWorld::move(EntityId src, EntityId dst, const ComponentSet& moveSet) {
  assert(exists(src));
  assert(exists(dst));

  const ComponentSet& entitySet = setOf(src);
  const ComponentSet& comps = moveSet.list().empty() ? entitySet : moveSet;
  add(dst, comps);

  for (ComponentId cid : comps.list()) {
    auto moveCtor = m_componentMoveCtor[cid];

    // TODO This is not really a full move, components use
    // their default ctor and then get move assigned, which may not be what you want.
    if (moveCtor)
      moveCtor(get(src, cid), get(dst, cid));
  }
}

const ComponentSet& EntityWorld::setOf(EntityId entity) {
  return *getEntitySet(entity);
}

ComponentId EntityWorld::component(ComponentId compId, bool isRegistered, const std::string_view& name, 
  size_t size, NewPoolFunc newPoolFunc, FromJsonFunc fromJsonFunc, ComponentCopyCtor copyCtor, ComponentMoveCtor moveCtor,
  SerializeFunc serializeFunc, DeserializeFunc deserializeFunc) noexcept {
  if (compId < m_componentNames.size())
    return compId;
  
  if(isRegistered)
    throw std::runtime_error("Component not initialized, despite being marked so");

  m_componentNames.resize(compId + 1, "");
  m_componentCopyCtor.resize(compId + 1, nullptr);
  m_componentMoveCtor.resize(compId + 1, nullptr);
  m_componentPools.resize(compId + 1, nullptr);

  m_componentNames[compId] = name;
  if (size != 0) {
    m_componentPools[compId] = newPoolFunc();
    m_componentCopyCtor[compId] = copyCtor;
    m_componentMoveCtor[compId] = moveCtor;
  }

  // Necessary for modules.
  findOrCreateSet({component<Enabled>(), compId});

  if(serializeFunc && deserializeFunc) {  
    m_serializer.registerComponent(compId, serializeFunc);
    m_deserializer.registerComponent(compId, deserializeFunc);
  }

  if(fromJsonFunc)
    m_assetLoader->registerComponent(m_self, compId, fromJsonFunc);

  return compId;
}

void EntityWorld::observe(ComponentId eventType, ComponentId comp, const ComponentSet& with, ObserverCallback callback) {
  m_observers[eventType][comp].emplace_back(ObserverData(findOrCreateSet(with), callback));
}

void EntityWorld::emit(ComponentId eventType, EntityId id, ComponentId comp) {
  for (ObserverData& observer : m_observers[eventType][comp]) {
    if (getEntitySet(id)->superset(*observer.with)) {
      observer.callback(getEntity(id));
    }
  }
}

AssetLoader EntityWorld::getAssetLoader() {
  return AssetLoader(m_self, *m_assetLoader);
}

Serializer& EntityWorld::getSerializer() {
  return m_serializer;
}

Deserializer& EntityWorld::getDeserializer() {
  return m_deserializer;
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
