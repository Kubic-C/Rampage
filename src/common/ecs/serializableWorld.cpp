#include "serializableWorld.hpp"

#include "capnp/pretty-print.h"
#include "capnp/serialize-packed.h"

#include "capnp/dynamic.h"
#include "capnp/compat/json.h"

#include "../ihost.hpp"

RAMPAGE_START

SerializableEntityWorld::SerializableEntityWorld(IHost& host, PrivateConstructorTag)
  : EntityWorld(host, PrivateConstructorTag{}) {}

ComponentId SerializableEntityWorld::component(ComponentId compid, bool isRegistered, const std::string& name, 
  size_t size, NewPoolFunc newPoolFunc, ComponentCopyCtor copyCtor, ComponentMoveCtor moveCtor,
  SerializeFunc serializeFunc, DeserializeFunc deserializeFunc) noexcept {
  ComponentId id = EntityWorld::component(compid, isRegistered, name, size, newPoolFunc, copyCtor, moveCtor, serializeFunc, deserializeFunc);
  registerSerializable(id, serializeFunc, deserializeFunc);
  return id;
}

void SerializableEntityWorld::registerSerializable(ComponentId compId, SerializeFunc serializeFunc, DeserializeFunc deserializeFunc) {
  m_serializer.registerComponent(compId, serializeFunc);
  m_deserializer.registerComponent(compId, deserializeFunc);
}

bool SerializableEntityWorld::saveState(const char* path, ComponentSet saveSet) {
  std::vector<std::pair<EntityId, ComponentSet>> entitiesToSerialize;
  auto iter = getWith(saveSet);
  while(iter->hasNext()) {
    auto entity = iter->next();
    entitiesToSerialize.emplace_back(entity.id(), setOf(entity.id()));
  }

  capnp::MallocMessageBuilder msg;
  auto rootBuilder = msg.initRoot<Schema::State>();
  m_host.getHostMutex().lock();
  m_serializer.serialize(*m_self, entitiesToSerialize, rootBuilder);
  m_host.getHostMutex().unlock();

  return m_serializer.serialize(msg, path);
}

bool SerializableEntityWorld::loadState(const char* path) {
  return m_deserializer.deserialize(*m_self, path);
}

RAMPAGE_END
