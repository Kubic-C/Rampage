#pragma once

#include "iworld.hpp"

RAMPAGE_START

class EntityWorld : public IWorld {
  friend class EntityPtr;
  friend class System;
  friend class Ref;
  friend class EntityIterator;
  friend class SetIterator;

  EntityWorld(const EntityWorld& other) = delete;
  EntityWorld(EntityWorld&& other) = delete;
  EntityWorld& operator=(const EntityWorld& other) = delete;
  EntityWorld& operator=(EntityWorld&& other) = delete;
  EntityWorld& operator=(EntityWorld& other) = delete;
  EntityWorld& operator=(EntityWorld other) = delete;

  struct PrivateConstructorTag {};

protected:
  using EntityList = std::list<EntityId, boost::fast_pool_allocator<EntityId>>;
  using EntityListIterator = EntityList::const_iterator;

public:
  using IWorld::component;
  using IWorld::set;
  using IWorld::observe;
  using IWorld::emit;
  using IWorld::add;
  using IWorld::remove;

  static IWorldPtr createWorld(IHost& host) {
    IWorldPtr world = std::make_shared<EntityWorld>(host, PrivateConstructorTag{});
    std::static_pointer_cast<EntityWorld>(world)->m_self = world;
    return world;
  }

  explicit EntityWorld(IHost& host, PrivateConstructorTag);
  virtual ~EntityWorld();

  virtual IHost& getHost() override;

  virtual void addContext(ContextId id, u8* bytes, std::function<void(u8*)> destroy) noexcept override;
  virtual u8* getContext(ContextId id) override;

  virtual EntityPtr create(EntityId explicitId = NullEntityId) override;
  virtual EntityPtr getEntity(EntityId id) override;
  virtual EntityPtr ensure(EntityId id) override;
  virtual bool exists(EntityId id) override;
  virtual bool isAlive(EntityId id) override;
  virtual void destroy(EntityId id) override;
  virtual void enable(EntityId entity) override;
  virtual void disable(EntityId entity) override;
  virtual EntityPtr clone(EntityId entity) override;
  virtual size_t getEntityCount() const override;
  virtual size_t getSetCount() const override;

  virtual void removeAll(const ComponentSet& components, bool notify = true) override;
  virtual void destroyAllEntitiesWith(const ComponentSet& components, bool notify = true) override;
  virtual IEntityIteratorPtr getWith(const ComponentSet& components) override;
  virtual IEntityIteratorPtr getWithDisabled(const ComponentSet& components) override;
  virtual EntityPtr getFirstWith(const ComponentSet& components) override;

  virtual void setLocalRange(EntityId startingId) override;
  virtual void enableRangeCheck(bool enable) override;
  virtual bool validRange(EntityId id) override;

  virtual void beginDefer() override;
  virtual void endDefer() override;
  virtual bool isDefer() override;

  virtual Ref get(EntityId entity, ComponentId compId) override;
  virtual void add(EntityId entity, const ComponentSet& addComps, bool emit = true) override;
  virtual void remove(EntityId entity, const ComponentSet& remComps, bool emit = true) override;
  virtual bool has(EntityId entity, ComponentId comp) override;
  virtual void copy(EntityId src, EntityId dst, const ComponentSet& comps = {}) override;
  virtual void move(EntityId src, EntityId dst, const ComponentSet& comps = {}) override;
  virtual const ComponentSet& setOf(EntityId entity) override;

  virtual ComponentId component(ComponentId compid, bool isRegistered, const std::string& name, 
    size_t size, NewPoolFunc newPoolFunc, ComponentCopyCtor copyCtor, ComponentMoveCtor moveCtor,
    SerializeFunc serializeFunc, DeserializeFunc deserializeFunc) noexcept override;

  virtual void observe(ComponentId eventType, ComponentId comp, const ComponentSet& with, ObserverCallback callback) override;
  virtual void emit(ComponentId eventType, EntityId entity, ComponentId comp) override;

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
  const ComponentSet* getEntitySet(EntityId id);
  void addToSuperSets(const ComponentSet* baseSet);

  virtual void registerSerializable(ComponentId compId, SerializeFunc serializeFunc, DeserializeFunc deserializeFunc) {}

protected:
  IWorldPtr m_self;
  IHost& m_host;

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
  Map<ComponentId, Map<ComponentId, std::vector<ObserverData>>> m_observers;

  std::vector<std::string> m_componentNames;
  std::vector<void (*)(u8* dst, u8* src)> m_componentCopyCtor;
  std::vector<void (*)(u8* dst, u8* src)> m_componentMoveCtor;
  std::vector<IPool*> m_componentPools;

  // A map that keeps track of super sets, meaning sets that contain the same or
  // more components
  Map<ComponentSet, const ComponentSet*> m_componentSets;
  Map<const ComponentSet*, Set<const ComponentSet*>> m_superSets;
  Map<const ComponentSet*, EntityList> m_sets;
};

RAMPAGE_END
