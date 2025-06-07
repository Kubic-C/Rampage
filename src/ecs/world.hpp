#pragma once

#include "../utility/log.hpp"
#include "componentSet.hpp"
#include "module.hpp"
#include "pool.hpp"

class Ref;
class Entity;
class System;

class EntityWorld {
  public:
  using EntityList = std::list<EntityId, boost::fast_pool_allocator<EntityId>>;
  using EntityListIterator = EntityList::const_iterator;

  enum class EventType { Add, Remove };

  struct ModuleData {
    std::shared_ptr<Module> m_module;
  };

  struct Destroy {};

  struct Enabled {};

  private:
  struct ContextData {
    u8* bytes = nullptr;
    std::function<void(u8*)> destroy;
  };

  struct ComponentGen {};

  struct ContextGen {};

  template <typename T>
  class StaticId {
public:
    template <typename X>
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
      if (this == &other)
        return *this;

      callback = other.callback;
      with = other.with;

      return *this;
    }

    ObserverData(const ComponentSet* with, ObserverCallback observer) : with(with), callback(observer) {}

    ObserverCallback callback;
    const ComponentSet* with;
  };

  using SystemFunc = std::function<void(Entity entity, float deltaTime)>;

  template <typename T>
  struct ModuleType {};

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

      return *this;
    }

    SetIterator(EntityWorld& world, const ComponentSet* match);

    void reset();
    bool hasNext() const;
    EntityList& next();
    EntityWorld& getWorld();

private:
    EntityWorld* m_world;
    const ComponentSet* m_base;
    Set<const ComponentSet*>::const_iterator m_next;
  };

  class EntityIterator {
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = Entity;
    using difference_type = std::ptrdiff_t;
    using pointer = Entity*;
    using reference = Entity&;

    EntityIterator& operator=(const EntityIterator& other) {
      m_setIt = other.m_setIt;
      m_next = other.m_next;
      m_list = other.m_list;
      return *this;
    }

    EntityIterator(SetIterator setIt);

    void reset();
    bool hasNext() const;
    Entity next();
    EntityWorld& getWorld();

private:
    EntityList* m_list;
    EntityListIterator m_next;
    SetIterator m_setIt;
  };

  EntityWorld();
  EntityWorld(const EntityWorld& other) = delete;
  EntityWorld(EntityWorld&& other) = delete;
  ~EntityWorld();

  EntityWorld& operator=(const EntityWorld& other) = delete;
  EntityWorld& operator=(EntityWorld&& other) = delete;
  EntityWorld& operator=(EntityWorld& other) = delete;
  EntityWorld& operator=(EntityWorld other) = delete;

  template <typename T, typename... Params>
  Entity addModule(Params&&... args);

  template <typename T>
  void enableModule();

  template <typename T>
  void disableModule();

  template <typename T>
  T& getModule();

  template <typename T, typename... Params>
  void addContext(Params&&... args) {
    int id = StaticId<ContextGen>::id<T>();

#ifndef NDEBUG
    logGeneric("Added CONTEXT: %i at %s\n", id, typeid(T).name());
#endif
    m_contexts[id].bytes = (u8*)new T(args...);
    m_contexts[id].destroy = [](u8* bytes) { delete (T*)bytes; };
    m_contextsLifo.push_back(id);
  }

  template <typename T>
  T& getContext() {
    int id = StaticId<ContextGen>::id<T>();
    assert(m_contexts.contains(id));
    return *(T*)m_contexts[id].bytes;
  }

  template <typename T>
  ComponentId component(const std::string_view& name = "") {
    ComponentId compId = StaticId<ComponentGen>::id<T>();
    if (compId < m_componentPools.size())
      return compId;

    m_componentNames.resize(compId + 1, "");
    m_componentCopyCtor.resize(compId + 1, nullptr);
    m_componentPools.resize(compId + 1, nullptr);

    if (name.empty())
      m_componentNames[compId] = boost::typeindex::type_id<T>().pretty_name();
    else
      m_componentNames[compId] = name;

    size_t size = sizeof(T);
    if constexpr (std::is_empty_v<T>)
      size = 0;
    if (size != 0) {
      m_componentPools[compId] = SparsePool<T>::createPool();

      m_componentCopyCtor[compId] = [](u8* dst, u8* src) { new ((T*)dst) T(*(T*)src); };
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

  size_t getEntityCount() const { return m_idMgr.count(); }

  size_t getSetCount() const { return m_sets.size(); }

  template <typename... Params>
  ComponentSet set() {
    return ComponentSet({component<Params>()...});
  }

  protected:
  inline IPool* getPool(ComponentId id) {
    assert(id < m_componentPools.size());
    return m_componentPools[id];
  }

  void moveSets(EntityId id, const ComponentSet* to);
  void tryMoveSets(EntityId id, const ComponentSet* to);
  const ComponentSet* findOrCreateSet(const ComponentSet& base);
  void notify(EventType type, EntityId entity, ComponentId compId);
  bool hasComponent(EntityId entity, ComponentId comp);
  const ComponentSet* getEntitySet(EntityId id);
  void addToSuperSets(const ComponentSet* baseSet);

  /* Deferred operations */
  bool m_isDefer = false;
  std::vector<const ComponentSet*> m_deferredSuperSetCalc;
  Map<EntityId, const ComponentSet*> m_deferredMoves;
  std::vector<EntityId> m_deferredDestroy;

  /* Contexts */
  std::vector<u32> m_contextsLifo;
  Map<u32, ContextData> m_contexts;

  /* Entities */
  std::vector<std::optional<EntityData>> m_entities;
  IdManager<EntityId> m_idMgr;

  /* Components */
  Map<EventType, Map<ComponentId, std::vector<ObserverData>>> m_observers;

  std::vector<std::string> m_componentNames;
  std::vector<void (*)(u8* dst, u8* src)> m_componentCopyCtor;
  std::vector<IPool*> m_componentPools;

  // A map that keeps track of super sets, meaning sets that contain the same or
  // more components
  Map<ComponentSet, const ComponentSet*> m_componentSets;
  Map<const ComponentSet*, Set<const ComponentSet*>> m_superSets;
  Map<const ComponentSet*, EntityList> m_sets;
};

using EntityIterator = EntityWorld::EntityIterator;
