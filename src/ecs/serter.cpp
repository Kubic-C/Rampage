#include "serter.hpp"

#include "capnp/pretty-print.h"
#include "capnp/serialize-packed.h"

void EntityWorldSerializable::registerSerializable(ComponentId compId, SerializeFunc serializeFunc, DeserializeFunc deserializeFunc) {
  if (m_componentSerializeFuncs.size() <= compId)
    m_componentSerializeFuncs.resize(compId + 1, nullptr);
  if (m_componentDeserializeFuncs.size() <= compId)
    m_componentDeserializeFuncs.resize(compId + 1, nullptr);

  m_componentSerializeFuncs[compId] = serializeFunc;
  m_componentDeserializeFuncs[compId] = deserializeFunc;
}

bool EntityWorldSerializable::saveState(const char* path, ComponentSet saveSet) {
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

    auto compIds = e.set().list(); // yes, copy
    const kj::ArrayPtr<ComponentId> kjCompIds(compIds.data(), compIds.size());
    entityBuilder.setCompIds(kjCompIds);
    auto compListBuilder = entityBuilder.initCompData(compIds.size());

    for (int i = 0; i < compIds.size(); ++i) {
      ComponentId compId = compIds[i];
      capnp::MallocMessageBuilder compBuilderMsg(m_scratchBuffer);

      // Serialize what we can.
      if (m_componentSerializeFuncs[compId])
        m_componentSerializeFuncs[compId](compBuilderMsg, e.get(compId));
      else
        compBuilderMsg.initRoot<Schema::Void>();

      scratchCompStream.clear();
      capnp::writePackedMessage(scratchCompStream, compBuilderMsg);
      size_t outputSize = scratchCompStream.getArray().size();
      auto dataBuilder = compListBuilder.init(i, outputSize);
      std::memcpy(dataBuilder.asBytes().begin(), scratchCompStream.getArray().begin(), outputSize);
    }
  }

  capnp::writePackedMessageToFd(_fileno(file), msg);
  fclose(file);

  // Print root to a string tree
  kj::StringTree tree = capnp::prettyPrint(root);
  std::string debugOutput = tree.flatten().cStr();
  std::cout << debugOutput << std::endl;

  return true;
}

bool EntityWorldSerializable::loadState(const char* path, bool overwrite) {
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

    if (overwrite && exists(eid)) {
      destroy(eid);
    }
    Entity e = ensure(eid); // it exists

    for (int i = 0; i < compIds.size(); ++i) {
      ComponentId realCompId = serCompRegistry[compIds[i]];

      e.add(realCompId);
      Ref compRef = e.get(realCompId);

      auto compData = serCompsData[i].asBytes();
      kj::ArrayInputStream stream(compData);
      capnp::PackedMessageReader compReader(stream);

      if (m_componentDeserializeFuncs[realCompId] == nullptr) {
        logGeneric("Component Deserialization for this component not implemented: %u %s\n", realCompId, m_componentNames[realCompId].c_str());
        continue;
      }

      m_componentDeserializeFuncs[realCompId](compReader, compRef);
    }
  }

  fclose(file);

  return true;
}