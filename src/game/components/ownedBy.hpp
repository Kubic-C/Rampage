#pragma once

#include "../../common/common.hpp"

RAMPAGE_START

struct OwnedByComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component);
  static void deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component);
  static void fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& jsonValue);

  EntityId owner;
};

RAMPAGE_END