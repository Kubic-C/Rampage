#include "serter.hpp"

#include "capnp/pretty-print.h"
#include "capnp/serialize-packed.h"

#include "../ihost.hpp"

RAMPAGE_START

void EntityWorldSerializable::registerSerializable(ComponentId compId, SerializeFunc serializeFunc,
                                                   DeserializeFunc deserializeFunc) {
  if (m_componentSerializeFuncs.size() <= compId)
    m_componentSerializeFuncs.resize(compId + 1, nullptr);
  if (m_componentDeserializeFuncs.size() <= compId)
    m_componentDeserializeFuncs.resize(compId + 1, nullptr);

  m_componentSerializeFuncs[compId] = serializeFunc;
  m_componentDeserializeFuncs[compId] = deserializeFunc;
}

bool EntityWorldSerializable::saveState(const char* path, ComponentSet saveSet) {
  std::set<std::string> nameOfComponentsNotSerializable;

  FILE* file;
  errno_t error = fopen_s(&file, path, "wb");
  if (error != 0)
    return false;

  capnp::MallocMessageBuilder msg;
  auto root = msg.initRoot<Schema::State>();

  // The first component is a special type of component, known as a NullComponent.
  // This should not be serialized.
  root.initRegisteredComponents(m_componentNames.size() - 1);
  for (ComponentId i = 1; i < m_componentNames.size(); ++i) {
    auto compBuilder = root.getRegisteredComponents()[i - 1];
    compBuilder.setName(m_componentNames[i]);
    compBuilder.setCompId(i);
  }

  size_t entityCount = 0;
  EntityIterator it = getWith(saveSet);
  while (it.hasNext()) {
    Entity e = it.next();
    entityCount++;
  }
  root.initEntities(entityCount);

  kj::VectorOutputStream scratchCompStream;
  uint32_t entityI = 0;
  it = getWith(saveSet);
  while (it.hasNext()) {
    Entity e = it.next();
    auto entityBuilder = root.getEntities()[entityI++];

    entityBuilder.setId(e.id());

    auto compIds = e.set().list(); // yes, copy
    const kj::ArrayPtr<ComponentId> kjCompIds(compIds.data(), compIds.size());
    entityBuilder.setCompIds(kjCompIds);
    auto compListBuilder = entityBuilder.initCompData(compIds.size());

    for (int i = 0; i < compIds.size(); ++i) {
      ComponentId compId = compIds[i];
      capnp::MallocMessageBuilder compBuilderMsg(m_scratchBuffer);

      if (compId >= m_componentSerializeFuncs.size() || !m_componentSerializeFuncs[compId]) {
        nameOfComponentsNotSerializable.insert(m_componentNames[compId]);
        continue;
      }

      m_componentSerializeFuncs[compId](compBuilderMsg, e.get(compId));

      scratchCompStream.clear();
      capnp::writePackedMessage(scratchCompStream, compBuilderMsg);
      size_t outputSize = scratchCompStream.getArray().size();
      auto dataBuilder = compListBuilder.init(i, outputSize);
      std::memcpy(dataBuilder.asBytes().begin(), scratchCompStream.getArray().begin(), outputSize);
    }
  }

  capnp::writePackedMessageToFd(_fileno(file), msg);
  fclose(file);

  m_host.log("List of components wo/ serializable\n");
  for(auto& name : nameOfComponentsNotSerializable) {
    m_host.log("\t%s\n", name.c_str());
  }

  return true;
}

bool EntityWorldSerializable::loadState(const char* path, bool appendEntities, bool clearPrevious) {
  FILE* file;
  errno_t error = fopen_s(&file, path, "rb");
  if (error != 0)
    return false;

  // sparse mapping of serialized comp ids to this worlds comp ids.
  // [SerCompId] -> Matching current World Id.
  // This is found by names
  std::vector<ComponentId> serCompRegistry;

  capnp::PackedFdMessageReader reader(_fileno(file));

  auto state = reader.getRoot<Schema::State>();

  for (auto comp : state.getRegisteredComponents()) {
    // o(n) search to find components names, too lazy to find something faster.
    for (int i = 0; i < m_componentNames.size(); ++i) {
      if (comp.getName() == m_componentNames[i]) {
        if (serCompRegistry.size() <= comp.getCompId())
          serCompRegistry.resize(comp.getCompId() + 1, 0);
        serCompRegistry[comp.getCompId()] = i;
      }
    }
  }

  for (auto entity : state.getEntities()) {
    EntityId eid = entity.getId();
    auto serCompsData = entity.getCompData();
    auto compIds = entity.getCompIds();

    if (eid == NullEntityId)
      continue;

    if (!appendEntities && exists(eid)) {
      if (clearPrevious)
        destroy(eid);
    } else {
      eid = NullEntityId;
    }
    Entity e = ensure(eid); // it exists

    for (int i = 0; i < compIds.size(); ++i) {
      ComponentId realCompId = serCompRegistry[compIds[i]];
      if (realCompId >= m_componentDeserializeFuncs.size() ||
          m_componentDeserializeFuncs[realCompId] == nullptr)
        continue;

      e.add(realCompId);
      Ref compRef = e.get(realCompId);

      auto compData = serCompsData[i].asBytes();
      kj::ArrayInputStream stream(compData);
      capnp::PackedMessageReader compReader(stream);

      m_componentDeserializeFuncs[realCompId](compReader, compRef);
    }
  }

  fclose(file);

  return true;
}

RAMPAGE_END
