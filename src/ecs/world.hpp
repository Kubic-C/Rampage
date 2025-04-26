#pragma once

#include "../utility/log.hpp"
#include "pool.hpp"
#include "module.hpp"
#include "componentSet.hpp"

class Ref;
class Entity;
class System;

class EntityWorld {
public:
  using EntityList = std::list<EntityId, boost::fast_pool_allocator<EntityId>>;
  using EntityListIterator = EntityList::const_iterator;

  enum class EventType {
    Add,
    Remove
  };

  struct ModuleData {
    std::shared_ptr<Module> m_module;
  };

private:
  struct ContextData {
    u8* bytes = nullptr;
    std::function<void(u8*)> destroy;
  };

  class ComponentIdCounter {
  public:
    template<typename T>
    static int id() {
      static int id = nextID();
      return id;
    }

  private:
    static int nextID() {
      static int counter = 0;
      return counter++;
    }
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

      return *this;
    }

    ObserverData(const ComponentSet* with, ObserverCallback observer)
      : with(with), callback(observer) {
    }

    ObserverCallback callback;
    const ComponentSet* with;
  };


  struct Destroy {};
  struct Enabled {};

  using SystemFunc = std::function<void(Entity entity, float deltaTime)>;

  template<typename T>
  struct ModuleType {};
private:
  friend class Entity;
  friend class System;
  friend class Ref;

public:
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
      return *(T*)it.next().get<ModuleData>()->m_module.get();

    throw std::exception("Module does not exist\n");
  }

  template<typename T, typename ... Params>
  void addContext(Params&& ... args) {
    m_contexts[typeid(T).hash_code()].bytes = (u8*)new T(args...);
    m_contexts[typeid(T).hash_code()].destroy = [](u8* bytes) { delete (T*)bytes;  };
    m_contextsLifo.push_back(typeid(T).hash_code());
  }
    
  //template<typename T>
  //void destroyContext() {
  //  size_t hashCode = typeid(T).hash_code();
  //  m_contexts[hashCode].destroy(m_contexts[hashCode].bytes);
  //  m_contexts.erase(hashCode);
  //}

  template<typename T>
  T& getContext() {
    return *(T*)m_contexts[typeid(T).hash_code()].bytes;
  }

  template<typename T>
  ComponentId component(const std::string_view& name = "") {
    ComponentId compId = ComponentIdCounter::id<T>();
    if (compId < m_componentPools.size())
      return compId;
    
    m_componentNames.resize(compId + 1, "");
    m_componentCopyCtor.resize(compId + 1, nullptr);
    m_componentPools.resize(compId + 1, nullptr);

    if (name == "")
      m_componentNames[compId] = boost::typeindex::type_id<T>().pretty_name();
    else
      m_componentNames[compId] = name;

    size_t size = sizeof(T);
    if constexpr (std::is_empty<T>::value)
      size = 0;
    if (size != 0) {
      m_componentPools[compId] = SparsePool<T>::createPool();

      m_componentCopyCtor[compId] = [](u8* dst, u8* src) {
          new((T*)dst)T(*(T*)src);
         };
    }

#ifndef NDEBUG
    logGeneric("Registered Comp: %s @ %u\n", m_componentNames[compId].c_str(), compId);
#endif

    // Necessary for modules.
    findOrCreateSet(set<Enabled, T>());

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
    return m_idMgr.count();
  }

  template<typename ... Params>
  ComponentSet set() {
    return ComponentSet({ component<Params>()... });
  }

protected:
  inline IPool* getPool(ComponentId id) {
    assert(id < m_components.size());
    return m_componentPools[id];
  }

  void moveSets(EntityId id, const ComponentSet* to);
  void tryMoveSets(EntityId id, const ComponentSet* to);
  const ComponentSet* findOrCreateSet(const ComponentSet& base);
  void notify(EventType type, EntityId entity, ComponentId compId);
  bool hasComponent(EntityId entity, ComponentId comp);
  const ComponentSet* getEntitySet(EntityId id);
  void addToSuperSets(const ComponentSet* baseSet);

private: 
  /* Deferred operations */
  bool m_isDefer = false;
  std::vector<const ComponentSet*> m_deferredSuperSetCalc;
  Map<EntityId, const ComponentSet*> m_deferredMoves;
  std::vector<EntityId> m_deferredDestroy;

  /* Contexts */
  std::vector<size_t> m_contextsLifo;
  Map<size_t, ContextData> m_contexts;

  /* Entities */
  std::vector<std::optional<EntityData>> m_entities;
  IdManager<EntityId> m_idMgr;

  /* Components */
  Map<EventType, Map<ComponentId, std::vector<ObserverData>>> m_observers;

  std::vector<std::string> m_componentNames;
  std::vector<void(*)(u8* dst, u8* src)> m_componentCopyCtor;
  std::vector<IPool*> m_componentPools;

  // A map that keeps track of super sets, meaning sets that contain the same or more components
  Map<ComponentSet, const ComponentSet*> m_componentSets;
  Map<const ComponentSet*, Set<const ComponentSet*>> m_superSets;
  Map<const ComponentSet*, EntityList> m_sets;

};

using EntityIterator = EntityWorld::EntityIterator;