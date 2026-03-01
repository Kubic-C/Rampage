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

  void serializeEntity(IWorld& world, EntityId eid, const ComponentSet& set, Schema::Entity::Builder entityBuilder);
  void serializeAssetEntity(IWorld& world, EntityId eid, const std::string& name, const ComponentSet& set, Schema::AssetEntity::Builder entityBuilder);
  void serializeComponentIdName(IWorld& world, ComponentId compId, Schema::ComponentIdName::Builder compIdNameBuilder);
  bool serializeToFile(capnp::MessageBuilder& builder, const char* path);

  template<IntegerIndexedContainer<ComponentId> Container>
  void serializeState(IWorld& world, Container entitiesToSerialize, Schema::State::Builder stateBuilder) {
    auto regCompsBuilder = stateBuilder.initRegisteredComponents((u32)m_componentSerializeFuncs.size());
    auto entitiesBuilder = stateBuilder.initEntities((u32)entitiesToSerialize.size());

    ComponentSetBuilder compSetBuilder;
    size_t i = 0;
    for (const auto&[entityId, compSet] : entitiesToSerialize) {
      serializeEntity(world, entityId, compSet, entitiesBuilder[i++]);
      compSetBuilder.add(compSet);
    }
    
    for (size_t i = 0; i < compSetBuilder.list().size(); ++i)
      serializeComponentIdName(world, compSetBuilder.list()[(u32)i], regCompsBuilder[(u32)i]);
  }

  template<IntegerIndexedContainer<ComponentId> Container>
  void serializeStateWithNames(IWorld& world, Container entitiesToSerialize, Schema::State::Builder stateBuilder) {
    auto regCompsBuilder = stateBuilder.initRegisteredComponents((u32)m_componentSerializeFuncs.size());
    auto entitiesBuilder = stateBuilder.initEntities((u32)entitiesToSerialize.size());

    ComponentSetBuilder compSetBuilder;
    size_t i = 0;
    for (const auto&[entityId, compSet, name] : entitiesToSerialize) {
      serializeAssetEntity(world, entityId, name, compSet, entitiesBuilder[i++]);
      compSetBuilder.add(compSet);
    }

    for (size_t i = 0; i < compSetBuilder.list().size(); ++i)
      serializeComponentIdName(world, compSetBuilder.list()[(u32)i], regCompsBuilder[(u32)i]);
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
  struct AssetEntityData {
    EntityId id;
    std::string name;
  };

  bool deserializeFromFile(IWorld& world, const char* path);
  bool deserializeStateFromData(IWorld& world, const std::vector<u8>& data);
  bool deserializeState(IWorld& world, Schema::State::Reader stateReader);
  void deserializeComponentList(IWorld& world, const IdMapper& idMapper, EntityId eid, capnp::List<u16>::Reader compIds, capnp::List<capnp::Data>::Reader compData);
  bool isRegistryValid(IWorld& world, capnp::List<Schema::ComponentIdName>::Reader regCompsReader);

  void registerComponent(ComponentId compId, DeserializeFunc deserializeFunc);

private:
  std::vector<DeserializeFunc> m_componentDeserializeFuncs;
};

RAMPAGE_END