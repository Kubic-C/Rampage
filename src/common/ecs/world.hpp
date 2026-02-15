#pragma once

#include <utility>

#include "componentSet.hpp"
#include "pool.hpp"
#include "staticId.hpp"
#include "capnp/message.h"

RAMPAGE_START

class Ref;
class Entity;
class System;
class IHost;
class EntityWorldSerializable;

struct ComponentAddedEvent {};
struct ComponentRemovedEvent {};

template<typename T>
concept SerializableComponent =
    requires(T t, capnp::MessageBuilder& builder, capnp::MessageReader& reader, Ref component) {
        T::serialize(builder, component);
        T::deserialize(reader, component);
    };
using SerializeFunc = void (*)(capnp::MessageBuilder& builder, Ref component);
using DeserializeFunc = void (*)(capnp::MessageReader& reader, Ref component);

class EntityWorld {
  friend class Entity;
  friend class System;
  friend class Ref;

  EntityWorld(const EntityWorld& other) = delete;
  EntityWorld(EntityWorld&& other) = delete;
  EntityWorld& operator=(const EntityWorld& other) = delete;
  EntityWorld& operator=(EntityWorld&& other) = delete;
  EntityWorld& operator=(EntityWorld& other) = delete;
  EntityWorld& operator=(EntityWorld other) = delete;

public:
  using EntityList = std::list<EntityId, boost::fast_pool_allocator<EntityId>>;
  using EntityListIterator = EntityList::const_iterator;

  struct Destroy {};
  struct Enabled {};

public:
  using SystemFunc = std::function<void(Entity entity, float deltaTime)>;
  using ObserverCallback = std::function<void(Entity)>;

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

public:
  explicit EntityWorld(IHost& host);
  virtual ~EntityWorld();

  IHost& getHost();

  template <typename T, typename... Params>
  void addContext(Params&&... args);

  template <typename T>
  T& getContext();

  /** Events
   * There are event types, an event type represents one possible event that can happen.
   *
   * When an event is "emitted" by an entity, all observers watching that specific event will trigger.
   * Additionally, observers trigger on an entity and a component. I.e. entity-component pairs.
   *
   * When an event is emitted:
   *  - Watching observers will trigger.
   *  - Additional context data will be passed down. This can be manipulated before calling emit.
   *
   * EventTypes are registered just like regular components.
   */

  template <typename T>
  ComponentId component(const std::string_view& name = "");

  template <typename EventType>
  void observe(ComponentId comp, const ComponentSet& with, ObserverCallback callback);

  template <typename EventType>
  void emit(EntityId entity, ComponentId comp);

  Entity create(EntityId explicitId = NullEntityId);
  Entity get(EntityId id);
  Entity ensure(EntityId id);
  bool exists(EntityId id);
  bool isAlive(EntityId id);
  EntityListIterator destroy(EntityId id);
  void enable(EntityId entity);
  void disable(EntityId entity);
  Entity clone(EntityId entity);
  size_t getEntityCount() const {
    return m_idMgr.count();
  }
  size_t getSetCount() const {
    return m_sets.size();
  }

  void removeAll(const ComponentSet& components, bool notify = true);
  void destroyAllEntitiesWith(const ComponentSet& components, bool notify = true);
  EntityIterator getWith(const ComponentSet& components);
  EntityIterator getWithDisabled(const ComponentSet& components);
  Entity getFirstWith(const ComponentSet& components);

  void setLocalRange(EntityId startingId);
  void enableRangeCheck(bool enable);
  bool validRange(EntityId id);

  void beginDefer();
  void endDefer();
  bool isDefer();

  template <typename... Params>
  ComponentSet set() {
    return ComponentSet({component<Params>()...});
  }

protected:
  struct ContextData {
    u8* bytes = nullptr;
    std::function<void(u8*)> destroy;
  };

  struct EntityData {
    const ComponentSet* comps = nullptr;
    EntityListIterator pos = EntityListIterator();
  };

  struct ObserverData {
    ObserverData& operator=(const ObserverData& other) {
      if (this == &other)
        return *this;

      callback = other.callback;
      with = other.with;

      return *this;
    }

    ObserverData(const ComponentSet* with, ObserverCallback observer) :
        with(with), callback(std::move(observer)) {}

    ObserverCallback callback;
    const ComponentSet* with;
  };

  template <typename T>
  struct ModuleType {};

  inline IPool* getPool(ComponentId id) {
    assert(id < m_componentPools.size());
    return m_componentPools[id];
  }

  void moveSets(EntityId id, const ComponentSet* to);
  void tryMoveSets(EntityId id, const ComponentSet* to);
  const ComponentSet* findOrCreateSet(const ComponentSet& base);
  bool hasComponent(EntityId entity, ComponentId comp);
  const ComponentSet* getEntitySet(EntityId id);
  void addToSuperSets(const ComponentSet* baseSet);

  virtual void registerSerializable(ComponentId compId, SerializeFunc serializeFunc, DeserializeFunc deserializeFunc) {}

protected:
  IHost& m_host;

  /* Deferred operations */
  bool m_isDefer = false;
  std::vector<const ComponentSet*> m_deferredSuperSetCalc;
  Map<EntityId, const ComponentSet*> m_deferredMoves;
  std::vector<EntityId> m_deferredDestroy;

  /* Contexts */
  StaticIdManager<u32> m_contextIdMgr;
  std::vector<u32> m_contextsLifo;
  Map<u32, ContextData> m_contexts;

  /* Entities */
  std::vector<std::optional<EntityData>> m_entities;
  IdManager<EntityId> m_idMgr;

  /* Components */
  StaticIdManager<ComponentId> m_componentIdMgr;
  Map<ComponentId, Map<ComponentId, std::vector<ObserverData>>> m_observers;

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

template <typename T, typename... Params>
void EntityWorld::addContext(Params&&... args) {
  const u32 id = m_contextIdMgr.id<T>();

  assert(!m_contexts.contains(id));

  m_contexts[id].bytes = reinterpret_cast<u8*>(new T(args...));
  m_contexts[id].destroy = [](u8* bytes) { delete reinterpret_cast<T*>(bytes); };
  m_contextsLifo.push_back(id);
}

template <typename T>
T& EntityWorld::getContext() {
  const u32 id = m_contextIdMgr.id<T>();
  assert(m_contexts.contains(id));
  return *reinterpret_cast<T*>(m_contexts[id].bytes);
}

template <typename T>
ComponentId EntityWorld::component(const std::string_view& name) {
  ComponentId compId = m_componentIdMgr.id<T>();
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

    if constexpr (std::is_copy_constructible_v<T>)
      m_componentCopyCtor[compId] = [](u8* dst, u8* src) { new ((T*)dst) T(*(T*)src); };
  }

  // Necessary for modules.
  findOrCreateSet(set<Enabled, T>());

  if constexpr (SerializableComponent<T>) {
    registerSerializable(compId,  T::serialize, T::deserialize);
  }

  return compId;
}

RAMPAGE_END
