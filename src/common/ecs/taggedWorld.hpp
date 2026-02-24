#pragma once

#include "iworld.hpp"

RAMPAGE_START

/**
 * A GameWorld is a proxy for another world that automatically adds component tags 
 * to all created entities. Any entity created through this GameWorld will be tagged 
 * with a unique component ID, allowing for easy querying and management of entities 
 * belonging to this specific game world, while still leveraging the core functionality 
 * of the underlying EntityWorld.
 */
class TaggedEntityWorld : public IWorld {
  struct PrivateConstructorTag {};
  
public:
  TaggedEntityWorld(IWorldPtr realWorld, ComponentId worldTagComponentId, PrivateConstructorTag);
  virtual ~TaggedEntityWorld() = default;
  
  static IWorldPtr create(IWorldPtr realWorld, ComponentId worldTagComponentId) {
    IWorldPtr world = std::make_shared<TaggedEntityWorld>(realWorld, worldTagComponentId, PrivateConstructorTag{});
    std::static_pointer_cast<TaggedEntityWorld>(world)->m_self = world;
    return world;
  }

  // Passthrough methods
  virtual IHost& getHost() override;
  virtual void addContext(ContextId id, u8* bytes, std::function<void(u8*)> destroy) noexcept override;
  virtual u8* getContext(ContextId id) override;

  // Entity creation with automatic tagging
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
  virtual bool has(EntityId entity, ComponentId compId) override;
  virtual void copy(EntityId src, EntityId dst, const ComponentSet& comps = {}) override;
  virtual void move(EntityId src, EntityId dst, const ComponentSet& comps = {}) override;
  virtual const ComponentSet& setOf(EntityId entity) override;

  virtual IPool* getPool(ComponentId id) override;
  virtual ComponentId component(ComponentId compId, bool isRegistered, const std::string& name,
    size_t size, NewPoolFunc newPoolFunc, ComponentCopyCtor copyCtor, ComponentMoveCtor moveCtor,
    SerializeFunc serializeFunc, DeserializeFunc deserializeFunc) noexcept override;

  virtual void observe(ComponentId eventType, ComponentId comp, const ComponentSet& with, ObserverCallback callback) override;
  virtual void emit(ComponentId eventType, EntityId entity, ComponentId comp) override;

private:
  IWorldPtr m_self;
  IWorldPtr m_realWorld;
  ComponentId m_worldTagComponentId;
};

RAMPAGE_END