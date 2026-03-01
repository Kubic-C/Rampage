#pragma once

#include "componentSet.hpp"
#include "ipool.hpp"
#include "id.hpp"

#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <capnp/pretty-print.h>
#include "../schema/rampage.capnp.h"

RAMPAGE_START

class Ref;
class EntityPtr;
class System;
class IHost;
class IWorld;
class EntityIterator;
class AssetLoader;
class IAssetLoaderImpl;
class Serializer;
class Deserializer;

struct ComponentAddedEvent {};
struct ComponentRemovedEvent {};

template<typename T>
concept SerializableComponent =
  requires(T t, capnp::MessageBuilder& builder, capnp::MessageReader& reader, const IdMapper& id, Ref component) {
    T::serialize(builder, component);
    T::deserialize(reader, id, component);
  };
using SerializeFunc = void (*)(capnp::MessageBuilder& builder, Ref component);
using DeserializeFunc = void (*)(capnp::MessageReader& reader, const IdMapper& id, Ref component);

template<typename T>
concept CopyableComponent =
  requires(T t, Ref src, Ref dst) {
    T::copy(src, dst);
  };

template<typename T>
concept MovableComponent =
  requires(T t, Ref src, Ref dst) {
    T::move(src, dst);
  };

template<typename T>
concept FromJsonComponent =
  requires(T t, Ref component, const json& json) {
    T::fromJson(component, json);
  };
using FromJsonFunc = void(*)(Ref component, AssetLoader loader, const json& json);

struct SerializableTag {
  static void serialize(capnp::MessageBuilder& builder, Ref component);
  static void deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component);
};

class IEntityIterator {
public:
  virtual ~IEntityIterator() = default;

  virtual void reset() = 0;
  virtual bool hasNext() const = 0;
  virtual EntityPtr next() = 0;
  virtual IWorld& getWorld() = 0;
};

using IEntityIteratorPtr = std::unique_ptr<IEntityIterator>;

class IWorld {
protected:
  struct PrivateConstructorTag {};

public:
  struct Destroy {};
  struct Enabled : SerializableTag {};

  using NewPoolFunc = IPool* (*)();
  using ComponentCopyCtor = void (*)(Ref src, Ref dst);
  using ComponentMoveCtor = void (*)(Ref src, Ref dst);
  using SystemFunc = std::function<void(EntityPtr entity, float deltaTime)>;
  using ObserverCallback = std::function<void(EntityPtr)>;

  template<typename T>
  ComponentCopyCtor getCopyCtor();

  template<typename T>
  ComponentMoveCtor getMoveCtor();

public:  
  virtual ~IWorld() = default;

  virtual IHost& getHost() = 0;
  virtual IWorld& getTopWorld() = 0;

  virtual void addContext(ContextId id, u8* bytes, std::function<void(u8*)> destroy) noexcept = 0;
  virtual u8* getContext(ContextId id) = 0;

  virtual EntityPtr create(EntityId explicitId = NullEntityId) = 0;
  virtual EntityPtr getEntity(EntityId id) = 0;
  virtual EntityPtr ensure(EntityId id) = 0;
  virtual bool exists(EntityId id)= 0;
  virtual bool isAlive(EntityId id) = 0;
  virtual void destroy(EntityId id) = 0;
  virtual void enable(EntityId entity) = 0;
  virtual void disable(EntityId entity) = 0;
  virtual EntityPtr clone(EntityId entity) = 0;
  virtual size_t getEntityCount() const = 0;
  virtual size_t getSetCount() const = 0;

  virtual void removeAll(const ComponentSet& components, bool notify = true) = 0;
  virtual void destroyAllEntitiesWith(const ComponentSet& components, bool notify = true) = 0;
  virtual IEntityIteratorPtr getWith(const ComponentSet& components) = 0;
  virtual IEntityIteratorPtr getWithDisabled(const ComponentSet& components) = 0;
  virtual EntityPtr getFirstWith(const ComponentSet& components) = 0;

  virtual void setLocalRange(EntityId startingId) = 0;
  virtual void enableRangeCheck(bool enable) = 0;
  virtual bool validRange(EntityId id) = 0;

  virtual void beginDefer() = 0;
  virtual void endDefer() = 0;
  virtual bool isDefer() = 0;

  virtual Ref get(EntityId entity, ComponentId compId) = 0;
  virtual std::string_view nameOf(ComponentId compId) = 0;
  virtual void add(EntityId entity, const ComponentSet& addComps, bool emit = true) = 0;
  virtual void remove(EntityId entity, const ComponentSet& remComps, bool emit = true) = 0;
  virtual bool has(EntityId entity, ComponentId compId) = 0;
  virtual void copy(EntityId src, EntityId dst, const ComponentSet& comps = {}) = 0;
  virtual void move(EntityId src, EntityId dst, const ComponentSet& comps = {}) = 0;
  virtual const ComponentSet& setOf(EntityId entity) = 0;

  virtual IPool* getPool(ComponentId id) = 0;
  virtual ComponentId component(ComponentId compId, bool isRegistered, const std::string_view& name, 
    size_t size, NewPoolFunc newPoolFunc, FromJsonFunc fromJsonFunc, ComponentCopyCtor copyCtor, ComponentMoveCtor moveCtor,
    SerializeFunc serializeFunc, DeserializeFunc deserializeFunc) noexcept = 0;

  virtual void observe(ComponentId eventType, ComponentId comp, const ComponentSet& with, ObserverCallback callback) = 0;
  virtual void emit(ComponentId eventType, EntityId entity, ComponentId comp) = 0;

  virtual AssetLoader getAssetLoader() = 0;
  virtual Serializer& getSerializer() = 0;
  virtual Deserializer& getDeserializer() = 0;

  template <typename T>
  ComponentId component(bool isRegistered = true) noexcept {
    size_t size = sizeof(T);
    if constexpr (std::is_empty_v<T>)
      size = 0; 

    FromJsonFunc fromJsonFunc = nullptr;
    if constexpr (FromJsonComponent<T>) 
      fromJsonFunc = &T::fromJson;

    ComponentCopyCtor copyCtor = getCopyCtor<T>();
    ComponentMoveCtor moveCtor = getMoveCtor<T>();

    SerializeFunc serializeFunc = nullptr;
    DeserializeFunc deserializeFunc = nullptr;
    if constexpr (SerializableComponent<T>) {
      serializeFunc = &T::serialize;
      deserializeFunc = &T::deserialize; 
    }

    return component(m_componentIdMgr.id<T>(), isRegistered, getTypeName<T>(), size, &SparsePool<T>::createPool, fromJsonFunc, copyCtor, moveCtor, serializeFunc, deserializeFunc);
  }

  template <typename... Params>
  ComponentSet set() {
    return ComponentSet({component<Params>()...});
  }

  template <typename EventType>
  void observe(ComponentId comp, const ComponentSet& with, ObserverCallback callback) {
    observe(component<EventType>(), comp, with, std::move(callback));
  }

  template <typename EventType>
  void emit(EntityId id, ComponentId comp) {
    emit(component<EventType>(), id, comp);
  }

  template <typename ... Params>
  void add(EntityId entity, bool emit = true) {
    add(entity, set<Params...>(), emit);
  }

  template <typename ... Params>
  void remove(EntityId entity, bool emit = true) {
    remove(entity, set<Params...>(), emit);
  }

  template<typename ContextType, typename... Params>
  void addContext(Params&&... params) {
    ContextType* context = new ContextType(std::forward<Params>(params)...);
    addContext(m_contextIdMgr.id<ContextType>(), reinterpret_cast<u8*>(context), [context](u8* bytes) {
      delete context;
    });
  }

  template<typename ContextType>
  ContextType& getContext() {
    return *reinterpret_cast<ContextType*>(getContext(m_contextIdMgr.id<ContextType>()));
  }

protected: 
  static inline StaticIdManager<u32> m_contextIdMgr;
  static inline StaticIdManager<u32> m_componentIdMgr;
};

using IWorldPtr = std::shared_ptr<IWorld>;

RAMPAGE_END