#pragma once

#include "ecs.hpp"
#include "capnp/message.h"
#include "../schema/rampage.capnp.h"

/* An extension of entity world that defines a set of methods to serialize component data. */
class EntityWorldSerializable : public EntityWorld {
  static constexpr size_t m_maxScratchWordSize = 512;
public:
  EntityWorldSerializable()
    : m_scratchBuffer(new capnp::word[m_maxScratchWordSize], m_maxScratchWordSize) {
    std::memset(m_scratchBuffer.begin(), 0, m_scratchBuffer.size() * sizeof(capnp::word));
  }

  using SerializeFunc = void(*)(capnp::MessageBuilder& builder, Ref component);
  using DeserializeFunc = void(*)(capnp::MessageReader& reader, Ref component);

  template<typename T>
  void registerSerializable(SerializeFunc serializeFunc, DeserializeFunc deserializeFunc) {
    registerSerializable(component<T>(), serializeFunc, deserializeFunc);
  }

  void registerSerializable(ComponentId compId, SerializeFunc serializeFunc, DeserializeFunc deserializeFunc);

  // Saves the current state of serializable data with a particular set to a file.
  bool saveState(const char* path, ComponentSet saveSet);

private:
  kj::ArrayPtr<capnp::word> m_scratchBuffer;
  std::vector<SerializeFunc> m_componentSerializeFuncs;
  std::vector<DeserializeFunc> m_componentDeserializeFuncs;
};

