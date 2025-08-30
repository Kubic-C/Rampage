#pragma once

#include "ecs.hpp"
#include "capnp/message.h"
#include "../schema/rampage.capnp.h"

RAMPAGE_START

/* An extension of entity world that defines a set of methods to serialize component data. */
class EntityWorldSerializable : public EntityWorld {
  static constexpr size_t m_maxScratchWordSize = 512;
public:
  EntityWorldSerializable()
    : m_scratchBuffer(new capnp::word[m_maxScratchWordSize], m_maxScratchWordSize) {
    std::memset(m_scratchBuffer.begin(), 0, m_scratchBuffer.size() * sizeof(capnp::word));

    // issa' tag
    registerSerializable<Enabled>(
      [](capnp::MessageBuilder& builder, Ref component) {
        builder.initRoot<Schema::Void>();
      },
      [](capnp::MessageReader& reader, Ref component) {});
  }

  ~EntityWorldSerializable() override = default;

  using SerializeFunc = void(*)(capnp::MessageBuilder& builder, Ref component);
  using DeserializeFunc = void(*)(capnp::MessageReader& reader, Ref component);

  // Registers a serializable type, using its static methods: serialize and deserialize
  template<typename T>
  void registerSerializable() {
    registerSerializable(component<T>(), T::serialize, T::deserialize);
  }

  template<typename T>
  void registerSerializable(SerializeFunc serializeFunc, DeserializeFunc deserializeFunc) {
    registerSerializable(component<T>(), serializeFunc, deserializeFunc);
  }

  void registerSerializable(ComponentId compId, SerializeFunc serializeFunc, DeserializeFunc deserializeFunc);

  // Saves the current state of serializable data with a particular set to a file.
  bool saveState(const char* path, ComponentSet saveSet);

  // Loads the state from a file. if appendEntities is true, any new entities loaded in
  // from the save file will be created with a newly generated ID, regardless of what their
  // assigned id is in the save file. clearPrevious is only used if the former is false
  // if clearPrevious is true: if an entity is loaded and the world already contains
  // an entity designated at that ID, it will be cleared of all of its components and then loaded,
  // if false, it will simply append or overwrite the already existing components.
  bool loadState(const char* path, bool appendEntities, bool clearPrevious = false);

private:
  kj::ArrayPtr<capnp::word> m_scratchBuffer;
  std::vector<SerializeFunc> m_componentSerializeFuncs;
  std::vector<DeserializeFunc> m_componentDeserializeFuncs;
};

RAMPAGE_END