#include "serializer.hpp"
#include "ref.hpp"
#include "../ihost.hpp"

RAMPAGE_START

Serializer::Serializer()
  : m_scratchBuffer(new capnp::word[m_defaultScratchBufferSize], m_defaultScratchBufferSize) {
  std::fill(m_scratchBuffer.begin(), m_scratchBuffer.end(), 0);
}

void Serializer::begin(IWorldPtr interfaceToUse) {
  m_input = std::make_unique<Input>();
  m_input->interfaceToUse = interfaceToUse;
}

std::vector<u8> Serializer::end() {
  std::vector<u8> output;
  kj::VectorOutputStream kjOutput;

  serialize(*m_input->interfaceToUse, m_input->entitiesToSerialize, m_input->messageBuilder.initRoot<Schema::State>());
  capnp::writePackedMessage(kjOutput, m_input->messageBuilder);
  output.insert(output.end(), kjOutput.getArray().begin(), kjOutput.getArray().end());

  m_input = nullptr;

  return output;  
}

void Serializer::queue(EntityId eid, const ComponentSet& set) {
  if (!m_input)
    return;

  m_input->entitiesToSerialize.insert(std::make_pair(eid, set));
}

void Serializer::queueAllWith(const ComponentSet& set) {
  if (!m_input)
    return;

  auto iter = m_input->interfaceToUse->getWith(set);
  while(iter->hasNext()) {
    auto entity = iter->next();
    m_input->entitiesToSerialize.insert(std::make_pair(entity.id(), m_input->interfaceToUse->setOf(entity.id())));
  }
}

void Serializer::serialize(IWorld& world, EntityId eid, const ComponentSet& set, Schema::Entity::Builder entityBuilder) {
  entityBuilder.setId(eid);
  auto compIdsBuilder = entityBuilder.initCompIds(set.list().size());
  auto compBytesBuilder = entityBuilder.initCompData(set.list().size());
  for(int j = 0; j < set.list().size(); ++j) {
    ComponentId compId = set.list()[j];
    capnp::MallocMessageBuilder compMsgBuilder(m_scratchBuffer);

    compIdsBuilder.set(j, compId);
    m_componentSerializeFuncs[compId](compMsgBuilder, world.get(eid, compId));
    m_scratchCompBytesStream.clear();
    capnp::writePackedMessage(m_scratchCompBytesStream, compMsgBuilder);
    compBytesBuilder.set(j, capnp::Data::Reader(kj::arrayPtr(m_scratchCompBytesStream.getArray().begin(), m_scratchCompBytesStream.getArray().size())));
  }
}

void Serializer::serialize(IWorld& world, EntityId eid, const std::string& name, const ComponentSet& set, Schema::AssetEntity::Builder entityBuilder) {
  entityBuilder.setId(eid);
  entityBuilder.setName(name);

  auto compIdsBuilder = entityBuilder.initCompIds(set.list().size());
  auto compBytesBuilder = entityBuilder.initCompData(set.list().size());
  for(int j = 0; j < set.list().size(); ++j) {
    ComponentId compId = set.list()[j];
    capnp::MallocMessageBuilder compMsgBuilder(m_scratchBuffer);

    compIdsBuilder.set(j, compId);
    m_componentSerializeFuncs[compId](compMsgBuilder, world.get(eid, compId));
    m_scratchCompBytesStream.clear();
    capnp::writePackedMessage(m_scratchCompBytesStream, compMsgBuilder);
    compBytesBuilder.set(j, capnp::Data::Reader(kj::arrayPtr(m_scratchCompBytesStream.getArray().begin(), m_scratchCompBytesStream.getArray().size())));
  }
}

void Serializer::serialize(IWorld& world, ComponentId compId, Schema::ComponentIdName::Builder compIdNameBuilder) {
  compIdNameBuilder.setCompId(compId);
  compIdNameBuilder.setName(world.nameOf(compId));
}

bool serialize(capnp::MessageBuilder& builder, const char* path) {
  FILE* file;
  errno_t error = fopen_s(&file, path, "wb");
  if (error != 0)
    return false;
  capnp::writePackedMessageToFd(_fileno(file), builder);
  fclose(file);
  return true;
}

void Serializer::registerComponent(ComponentId compId, SerializeFunc serializeFunc) {
  if (compId >= m_componentSerializeFuncs.size())
    m_componentSerializeFuncs.resize(compId + 1, nullptr);
  m_componentSerializeFuncs[compId] = serializeFunc;
}

bool Deserializer::deserialize(IWorld& world, const char* path) {
  IHost& host = world.getHost();

  FILE* file;
  errno_t error = fopen_s(&file, path, "rb");
  if (error != 0)
    return false;
  capnp::PackedFdMessageReader reader(_fileno(file));

  auto state = reader.getRoot<Schema::State>();
  host.getHostMutex().lock();
  bool result = deserialize(world, state);
  host.getHostMutex().unlock();

  fclose(file);

  return result;
}

bool Deserializer::deserialize(IWorld& world, const std::vector<u8>& data) {
  kj::ArrayPtr<const capnp::byte> inputWords(reinterpret_cast<const capnp::byte*>(data.data()), data.size());
  kj::ArrayInputStream kjInput(inputWords);
  capnp::PackedMessageReader reader(kjInput);

  return deserialize(world, reader.getRoot<Schema::State>());
}

bool Deserializer::deserialize(IWorld& world, Schema::State::Reader stateReader) {
  auto regCompsReader = stateReader.getRegisteredComponents();
  auto entitiesReader = stateReader.getEntities();

  if(!isRegistryValid(world, regCompsReader))
    return false;

  for (auto entityReader : entitiesReader)
    deserialize(world, entityReader);

  return true;
}

void Deserializer::deserialize(IWorld& world, Schema::Entity::Reader entityReader) {
  EntityId eid = entityReader.getId();
  auto compIds = entityReader.getCompIds();
  auto compData = entityReader.getCompData();

  ComponentSetBuilder setBuilder;
  setBuilder.list().insert(setBuilder.list().end(), compIds.begin(), compIds.end());

  if(world.exists(eid))
    world.destroy(eid);
  world.create(eid);
  world.add(eid, setBuilder.build());

  for (int i = 0; i < compIds.size(); ++i) {
    ComponentId compId = compIds[i];
    kj::ArrayInputStream kjCompStream(compData[i].asBytes());
    capnp::PackedMessageReader compReader(kjCompStream);

    m_componentDeserializeFuncs[compId](compReader, world.get(eid, compId));
  }
}

bool Deserializer::isRegistryValid(IWorld& world, capnp::List<Schema::ComponentIdName>::Reader regCompsReader) {
  bool failedMatch = false;
  for (auto comp : regCompsReader) {
    auto compId = comp.getCompId();
    auto compName = comp.getName();

    if(world.nameOf(compId) != compName.cStr()) {
      world.getHost().log("Component with id %u in memory has name %s, but in current world has name %s\n", compId, compName.cStr(), world.nameOf(compId).c_str());
      failedMatch = true;
    } else if(m_componentDeserializeFuncs.size() <= compId || m_componentDeserializeFuncs[compId] == nullptr) {
      world.getHost().log("Component with id %u and name %s does not have a registered deserialization function\n", compId, compName.cStr());
      failedMatch = true;
    }
  }
  if (failedMatch) {
    world.getHost().log("Component registry failure, aborting deserialization\n");
    return false;
  }

  return true;
}

void Deserializer::registerComponent(ComponentId compId, DeserializeFunc deserializeFunc) {
  if (compId >= m_componentDeserializeFuncs.size())
    m_componentDeserializeFuncs.resize(compId + 1, nullptr);
  m_componentDeserializeFuncs[compId] = deserializeFunc;
}

RAMPAGE_END