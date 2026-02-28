#pragma once

#include "ref.hpp"
#include "world.hpp"
#include "serializer.hpp"

RAMPAGE_START

/* An extension of entity world that defines a set of methods to serialize component data. */
class SerializableEntityWorld: public EntityWorld {
  static constexpr size_t m_maxScratchWordSize = 512;

public:
  SerializableEntityWorld(IHost& host, PrivateConstructorTag);
  ~SerializableEntityWorld() noexcept override = default;
  
  static IWorldPtr createWorld(IHost& host) {
    IWorldPtr world = std::make_shared<SerializableEntityWorld>(host, PrivateConstructorTag{});
    std::static_pointer_cast<SerializableEntityWorld>(world)->m_self = world;
    return world;
  }

  virtual ComponentId component(ComponentId compid, bool isRegistered, const std::string& name, 
    size_t size, NewPoolFunc newPoolFunc, ComponentCopyCtor copyCtor, ComponentMoveCtor moveCtor,
    SerializeFunc serializeFunc, DeserializeFunc deserializeFunc) noexcept override;

  bool saveState(const char* path, ComponentSet saveSet);
  bool loadState(const char* path);

private:
  void registerSerializable(ComponentId compId, SerializeFunc serializeFunc, DeserializeFunc deserializeFunc);

  Serializer m_serializer;
  Deserializer m_deserializer;
}; 

RAMPAGE_END
