#include "ownedBy.hpp"

RAMPAGE_START

void OwnedByComponent::serialize(capnp::MessageBuilder& builder, Ref component) {
  auto ownedByBuilder = builder.initRoot<Schema::OwnedByComponent>();
  auto self = component.cast<OwnedByComponent>();

  ownedByBuilder.setOwner(self->owner);
}

void OwnedByComponent::deserialize(capnp::MessageReader& reader, const IdMapper& idMapper, Ref component) {
  auto ownedByReader = reader.getRoot<Schema::OwnedByComponent>();
  auto self = component.cast<OwnedByComponent>();

  self->owner = idMapper.resolve(ownedByReader.getOwner());
}

void OwnedByComponent::fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& jsonValue) {
  auto self = component.cast<OwnedByComponent>();
  auto compJson = jsonValue.as<JSchema::OwnedByComponent>();

  if (compJson->hasOwner())
    self->owner = loader.getAsset(compJson->getOwner());
}

RAMPAGE_END
