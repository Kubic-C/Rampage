#include "iworld.hpp"
#include "ref.hpp"

RAMPAGE_START

void SerializableTag::serialize(capnp::MessageBuilder& builder, Ref component) {
  builder.initRoot<Schema::Void>();
}

void SerializableTag::deserialize(capnp::MessageReader& reader, Ref component) {}

RAMPAGE_END