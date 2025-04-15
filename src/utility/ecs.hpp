#pragma once

#include "id.hpp"

using EntityId = u32;
using ComponentId = u16;
using ComponentSetId = u64;
static constexpr EntityId NullEntityId = 0;
static constexpr ComponentId NullComponentId = 0;

class IPool {
public:
  virtual u8* create(EntityId id) = 0;
  virtual void destroy(EntityId id) = 0;
  virtual bool exists(EntityId id) = 0;
  virtual u8* get(EntityId id) = 0;

  template<typename T>
  T& get(EntityId id) {
    return *(T*)get(id);
  }
};

template<typename T>
class Pool : public IPool {
public:
  static std::shared_ptr<IPool> createPool() {
    return std::make_shared<Pool<T>>();
  }

  u8* create(EntityId id) override {
    assert(!exists(id));
    m_objects[id] = m_pool.construct();
    return (u8*)m_objects.find(id)->second;
  }

  void destroy(EntityId id) override {
    assert(exists(id));
    m_pool.destroy(m_objects.find(id)->second);
    m_objects.erase(id);
  }

  bool exists(EntityId id) override {
    return m_objects.find(id) != m_objects.end();
  }

  u8* get(EntityId id)override {
    assert(exists(id));
    return (u8*)m_objects.at(id);
  }

private:
  Map<EntityId, T*> m_objects;
  boost::object_pool<T> m_pool;
};

class ComponentSetBuilder;
class ComponentSet {
  friend class ComponentSetBuilder;
public:
  ComponentSet() = default;
  ComponentSet(const ComponentSet&) = default;
  ComponentSet(ComponentSet&&) = default;

  ComponentSet(const std::vector<ComponentId>& ids);
  ComponentSet(const std::initializer_list<ComponentId>& ids);

  const ComponentSet add(ComponentId id) const;
  const ComponentSet remove(ComponentId id) const;
  bool has(ComponentId id) const;
  // Is this set a superset of other set
  bool superset(const ComponentSet& set) const;
  // Is this set a subset of other set
  bool subset(const ComponentSet& set) const;
  ComponentSetId getSetId() const;
  const std::vector<ComponentId>& list() const;
     
  ComponentSet& operator=(const ComponentSet& other) = default;

  bool operator==(const ComponentSet& other) const {
    return getSetId() == other.getSetId();
  }

protected:
  std::vector<ComponentId> m_comps;
};

class ComponentSetBuilder {
public:
  static const ComponentSet empty;

  ComponentSetBuilder() = default;
  ComponentSetBuilder(const ComponentSetBuilder&) = default;
  ComponentSetBuilder(ComponentSetBuilder&&) = default;

  ComponentSetBuilder(const ComponentSet& ids);
  ComponentSetBuilder(const std::initializer_list<ComponentId>& ids);

  const void add(ComponentId id);
  const void add(const ComponentSet& set);
  const void remove(ComponentId id);
  const void remove(const ComponentSet& set);
  bool has(ComponentId id) const;
  // Is this set a superset of other set
  bool superset(const ComponentSet& set) const;
  // Is this set a subset of other set
  bool subset(const ComponentSet& set) const;
  ComponentSetId getSetId() const;
  const std::vector<ComponentId>& list() const;
  ComponentSet build() const;

protected:
  std::vector<ComponentId> m_comps;
};

namespace boost {
  template <>
  struct hash<ComponentSet> {
    size_t operator()(const ComponentSet& set) const {
      return set.getSetId();
    }
  };
}

namespace std {
  template <>
  struct hash<ComponentSet> {
    size_t operator()(const ComponentSet& set) const {
      return set.getSetId();
    }
  };
}

class EntityWorld;

class Module {
public:
  virtual ~Module() {}

  virtual void run(EntityWorld& world, float deltaTime) {}
private:
};

class Entity;
class System;
class IteratedEntity;

class EntityWorld {
  friend class Entity;
  friend class System;

  using EntityList = std::list<EntityId, boost::fast_pool_allocator<EntityId>>;
public:
  using EntityListIterator = EntityList::const_iterator;
  class SetIterator {
  public:
    SetIterator& operator=(const SetIterator& other) {
      m_base = other.m_base;
      m_next = other.m_next;
      m_world = other.m_world;
    }

    SetIterator(EntityWorld& world, const ComponentSet* match);

    void reset();
    bool hasNext() const;
    EntityList& next();
    EntityWorld& getWorld();

  private:
    EntityWorld& m_world;
    const ComponentSet* m_base;
    Set<const ComponentSet*>::const_iterator m_next;
  };

  class EntityIterator {
  public:
    EntityIterator& operator=(const EntityIterator& other) {
      m_setIt = other.m_setIt;
      m_next = other.m_next;
      m_list = other.m_list;
    }

    EntityIterator(SetIterator setIt);

    void reset();
    bool hasNext() const;
    Entity next();
    EntityWorld& getWorld();

  private:
    EntityList* m_list;
    EntityListIterator m_next;
    EntityWorld::SetIterator m_setIt;
  };

  struct ContextData {
    u8* bytes;
    std::function<void(u8*)> destroy;
  };

  struct ComponentData {
    std::string name;
    void(*m_copyCtor)(u8* dst, u8* src) = nullptr;
    std::shared_ptr<IPool> pool;
  };

  struct EntityData {
    const ComponentSet* comps = nullptr;
    EntityListIterator pos = EntityListIterator();
  };

  using ObserverCallback = std::function<void(Entity)>;

  struct ObserverData {
    ObserverData& operator=(const ObserverData& other) {
      callback = other.callback;
      with = other.with;
    }

    ObserverData(const ComponentSet* with, ObserverCallback observer)
      : with(with), callback(observer) {}

    ObserverCallback callback;
    const ComponentSet* with;
  };

  enum class EventType {
    Add,
    Remove
  };

  struct Destroy {};
  struct Enabled {};

  using SystemFunc = std::function<void(Entity entity, float deltaTime)>;

  template<typename T>
  struct ModuleType {};

  struct ModuleData {
    std::shared_ptr<Module> m_module;
  };

public:
  EntityWorld();
  ~EntityWorld();

  template<typename T, typename ... Params>
  Entity addModule(Params&& ... args);

  template<typename T>
  void enableModule() {
    EntityIterator it = getWithDisabled(set<ModuleType<T>>());
    beginDefer();
    while (it.hasNext()) {
      it.next().add<Enabled>();
    }
    endDefer();
  }

  template<typename T>
  void disableModule() {
    EntityIterator it = getWithDisabled(set<ModuleType<T>>());
    beginDefer();
    while (it.hasNext()) {
      it.next().remove<Enabled>();
    }
    endDefer();
  }

  template<typename T>
  T& getModule() {
    EntityIterator it = getWithDisabled(set<ModuleType<T>>());
    while (it.hasNext())
      return *(T*)it.next().get<ModuleData>().m_module.get();

    throw std::exception("Module does not exist\n");
  }

  template<typename T, typename ... Params>
  void addContext(Params&& ... args) {
    m_contexts[typeid(T).hash_code()].bytes = (u8*)new T(args...);
    m_contexts[typeid(T).hash_code()].destroy = [](u8* bytes) { delete (T*)bytes;  };
  }

  template<typename T>
  void destroyContext() {
    size_t hashCode = typeid(T).hash_code();
    m_contexts[hashCode].destroy(m_contexts[hashCode].bytes);
    m_contexts.erase(hashCode);
  }

  template<typename T>
  T& getContext() {
    return *(T*)m_contexts[typeid(T).hash_code()].bytes;
  }

  template<typename T>
  ComponentId component(const std::string& name = boost::typeindex::type_id<T>().pretty_name()) {
    size_t size = sizeof(T);
    if constexpr (std::is_empty<T>::value)
      size = 0;

    const size_t hashId = typeid(T).hash_code();
    ComponentId compId = NullComponentId;
    if (m_localAliases.find(hashId) != m_localAliases.end()) {
      return m_localAliases[hashId];
    } else {
      compId = ++m_compIdCounter;
      m_localAliases[hashId] = compId;
    }

    ComponentData& compData = m_components[compId];
    compData.name = name;
    if (size != 0) {
      compData.pool = Pool<T>::createPool();

      compData.m_copyCtor = [](u8* dst, u8* src) {
          new((T*)dst)T(*(T*)src);
         };
    }

    return compId;
  }

  void observe(EventType event, ComponentId set, const ComponentSet& with, ObserverCallback callback);

  /**
   * Creates a new entity, or if explicitId is non-zero creates the
   * entity at that specified ID.  */
  Entity create(EntityId explicitId = NullEntityId);
  Entity get(EntityId id);

  /* If entity does not exist, create it @id, if it does get() it*/
  Entity ensure(EntityId id);

  /**
   * Returns true if the entity exists, entity may be tagged with destroy */
  bool exists(EntityId id);

  /**
   * Returns true if the entity is alive, meaning not marked with destroy */
  bool isAlive(EntityId id);

  /**
   * Destroy the entity */
  EntityListIterator destroy(EntityId id);

  void enable(EntityId entity);
  void disable(EntityId entity);

  // remove all type of components
  void removeAll(const ComponentSet& components, bool notify = true);
  // destroy all 
  void destroyAllEntitiesWith(const ComponentSet& components, bool notify = true);
  EntityIterator getWith(const ComponentSet& components);
  // Gets all entities regardless if they are disabled or enabled
  EntityIterator getWithDisabled(const ComponentSet& components);
  // Gets first found enabled entity with components
  Entity getFirstWith(const ComponentSet& components);

  // Run them thing homey
  void run(float deltaTime);
  System system(const ComponentSet& components, const SystemFunc& func);
  System systemWithDisabled(const ComponentSet& components, const SystemFunc& func);

  void setLocalRange(EntityId startingId);
  void enableRangeCheck(bool enable);
  bool validRange(EntityId id);

  void beginDefer();
  void endDefer();
  bool isDefer();

  Entity clone(EntityId entity);

  size_t getEntityCount() {
    return m_entities.size();
  }

  template<typename ... Params>
  ComponentSet set() {
    return ComponentSet({ component<Params>()... });
  }

protected:

  template<typename T>
  ComponentId alias() {
    return m_localAliases.find(typeid(T).hash_code())->second;
  }

  ComponentData& getComponentData(ComponentId id);
  void moveSets(EntityId id, const ComponentSet* to);
  void tryMoveSets(EntityId id, const ComponentSet* to);
  const ComponentSet* findOrCreateSet(const ComponentSet& base);
  void notify(EventType type, EntityId entity, ComponentId compId);
  bool hasComponent(EntityId entity, ComponentId comp);
  const ComponentSet* getEntitySet(EntityId id);

private: 
  bool m_isDefer = false;
  Map<EntityId, const ComponentSet*> m_deferredMoves;
  std::vector<EntityId> m_deferredDestroy;

  Map<EventType, Map<ComponentId, std::vector<ObserverData>>> m_observers;
  
  Map<size_t, ContextData> m_contexts;

  // A map that keeps track of super sets, meaning sets that contain the same or more components
  Map<const ComponentSet*, Set<const ComponentSet*>> m_superSets;
  Map<const ComponentSet*, EntityList> m_sets;
  Map<EntityId, EntityData> m_entities;
  IdManager<EntityId> m_idMgr;

  Map<ComponentId, ComponentData> m_components;
  Map<size_t, ComponentId> m_localAliases;

  Map<ComponentSet, const ComponentSet*> m_componentSets;

  ComponentId m_compIdCounter = 0;
};

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
  u8* get(ComponentId compId);
  bool has(ComponentId compId);
  EntityWorld& world();
  void enable();
  void disable();
  Entity clone();

  template<typename T>
  void add() {
    add(m_world.alias<T>());
  }

  template<typename T>
  void remove() {
    remove(m_world.alias<T>());
  }

  template<typename T>
  T& get() {
    return *(T*)get(m_world.alias<T>());

  }
  template<typename T>
  bool has() {
    return has(m_world.alias<T>());
  }

  operator EntityId() const {
    return m_id;
  }

private:
  EntityWorld& m_world;
  EntityId m_id;
};

template<typename T, typename ... Params>
Entity EntityWorld::addModule(Params&& ... args) {
  component<ModuleType<T>>();

  Entity moduleEntity = create();
  moduleEntity.add<ModuleData>();
  moduleEntity.add<ModuleType<T>>();
  ModuleData& moduleT = moduleEntity.get<ModuleData>();
  moduleT.m_module = std::make_shared<T>(*this, args...);
  
  disable(moduleEntity);

  return moduleEntity;
}

using EntityIterator = EntityWorld::EntityIterator;

struct System {
  friend class EntityWorld;

protected:
  System(const EntityIterator& it, const EntityWorld::SystemFunc& func) 
    : m_it(it), m_func(func) {}
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