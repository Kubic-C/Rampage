#pragma once

#include "ref.hpp"
#include "world.hpp"

RAMPAGE_START

/* An extension of entity world that defines a set of methods to serialize component data. */
class SerializableEntityWorld: public EntityWorld {
  static constexpr size_t m_maxScratchWordSize = 512;

public:
  SerializableEntityWorld(IHost& host, PrivateConstructorTag);
  ~SerializableEntityWorld() override = default;
  
  static IWorldPtr createWorld(IHost& host) {
    IWorldPtr world = std::make_shared<SerializableEntityWorld>(host, PrivateConstructorTag{});
    std::static_pointer_cast<SerializableEntityWorld>(world)->m_self = world;
    return world;
  }

  virtual ComponentId component(ComponentId compid, bool isRegistered, const std::string& name, 
    size_t size, NewPoolFunc newPoolFunc, ComponentCopyCtor copyCtor, ComponentMoveCtor moveCtor,
    SerializeFunc serializeFunc, DeserializeFunc deserializeFunc) noexcept override;

  // Saves the current state of serializable data with a particular set to a file.
  bool saveState(const char* path, ComponentSet saveSet);

  // if (clearPrevious && exists(eid))
  //   destroy(eid);
  // if(appendEntities)
  //   eid = NullEntityId;
  bool loadState(const char* path, bool appendEntities, bool clearPrevious = false);

private:
  void registerSerializable(ComponentId compId, SerializeFunc serializeFunc, DeserializeFunc deserializeFunc);

  kj::ArrayPtr<capnp::word> m_scratchBuffer;
  std::vector<SerializeFunc> m_componentSerializeFuncs;
  std::vector<DeserializeFunc> m_componentDeserializeFuncs;
}; 

RAMPAGE_END
