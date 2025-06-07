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