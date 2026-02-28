#pragma once

#include "iworld.hpp"

RAMPAGE_START

template<typename Container, typename Id>
concept IntegerIndexedContainer = 
requires(Container c, Id id) {
  c[id];
  { c.size() } -> std::convertible_to<size_t>;
};

class Serializer {
public:
  Serializer();

  void begin(IWorldPtr interfaceToUse);
  std::vector<u8> end();

  void queue(EntityId eid, const ComponentSet& set);
  void queueAllWith(const ComponentSet& set);

  void serialize(IWorld& world, EntityId eid, const ComponentSet& set, Schema::Entity::Builder entityBuilder);
  void serialize(IWorld& world, EntityId eid, const std::string& name, const ComponentSet& set, Schema::AssetEntity::Builder entityBuilder);
  void serialize(IWorld& world, ComponentId compId, Schema::ComponentIdName::Builder compIdNameBuilder);
  bool serialize(capnp::MessageBuilder& builder, const char* path);

  template<IntegerIndexedContainer<ComponentId> Container>
  void serialize(IWorld& world, Container entitiesToSerialize, Schema::State::Builder stateBuilder) {
    auto regCompsBuilder = stateBuilder.initRegisteredComponents(m_componentSerializeFuncs.size());
    auto entitiesBuilder = stateBuilder.initEntities(entitiesToSerialize.size());

    ComponentSetBuilder compSetBuilder;
    size_t i = 0;
    for (const auto& pair : entitiesToSerialize) {
      serialize(world, pair.first, pair.second, entitiesBuilder[i++]);
      compSetBuilder.add(pair.second);
    }

    for (size_t i = 0; i < compSetBuilder.list().size(); ++i)
      serialize(world, compSetBuilder.list()[i], regCompsBuilder[i]);
  }

  void registerComponent(ComponentId compId, SerializeFunc serializeFunc);

private:
  struct Input {
    IWorldPtr interfaceToUse;
    capnp::MallocMessageBuilder messageBuilder;
    OpenMap<EntityId, ComponentSet> entitiesToSerialize;
  };

  std::unique_ptr<Input> m_input;
  std::vector<SerializeFunc> m_componentSerializeFuncs;
  
  kj::VectorOutputStream m_scratchCompBytesStream;
  static constexpr size_t m_defaultScratchBufferSize = 1028;
  kj::ArrayPtr<capnp::word> m_scratchBuffer;

};

class Deserializer {
public:
  bool deserialize(IWorld& world, const char* path);
  bool deserialize(IWorld& world, const std::vector<u8>& data);
  bool deserialize(IWorld& world, Schema::State::Reader stateReader);
  void deserialize(IWorld& world, Schema::Entity::Reader reader);
  bool isRegistryValid(IWorld& world, capnp::List<Schema::ComponentIdName>::Reader regCompsReader);

  void registerComponent(ComponentId compId, DeserializeFunc deserializeFunc);

private:
  std::vector<DeserializeFunc> m_componentDeserializeFuncs;
};

RAMPAGE_END