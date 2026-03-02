#include "iworld.hpp"
#include "ref.hpp"
#include "iassetLoader.hpp"

RAMPAGE_START

void SerializableTag::serialize(capnp::MessageBuilder& builder, Ref component) {
  builder.initRoot<Schema::Void>();
}

void SerializableTag::deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component) {}

void JsonableTag::fromJson(Ref component, AssetLoader loader, const json& compJson) {}

RAMPAGE_END