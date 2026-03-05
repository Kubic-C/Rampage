#include "transform.hpp"

RAMPAGE_START

void TransformComponent::serialize(capnp::MessageBuilder& builder, Ref component) {
  auto transformBuilder = builder.initRoot<Schema::TransformComponent>();
  auto transform = component.cast<TransformComponent>();

  transformBuilder.getPos().setX(transform->pos.x);
  transformBuilder.getPos().setY(transform->pos.y);
  transformBuilder.setRot(transform->rot);
}

void TransformComponent::deserialize(capnp::MessageReader& reader, const IdMapper& idMapper,Ref component) {
  auto transformReader = reader.getRoot<Schema::TransformComponent>();
  auto transform = component.cast<TransformComponent>();

  // Validate position values
  float x = transformReader.getPos().getX();
  float y = transformReader.getPos().getY();
  
  // Check for NaN or infinity
  if (std::isnan(x) || std::isinf(x) || std::abs(x) > 1e6f) {
    x = 0.0f;
  }
  if (std::isnan(y) || std::isinf(y) || std::abs(y) > 1e6f) {
    y = 0.0f;
  }

  transform->rot = transformReader.getRot();
  transform->pos.x = x;
  transform->pos.y = y;
}

void TransformComponent::fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& jsonValue) {
  auto transform = component.cast<TransformComponent>();
  auto compJson = jsonValue.as<JSchema::TransformComponent>();

  if(compJson->hasPos()) {
    auto posJson = compJson->getPos();
    if(posJson.hasX())
      transform->pos.x = posJson.getX();
    if(posJson.hasY())
      transform->pos.y = posJson.getY();
  }
  if(compJson->hasRot())
    transform->rot = compJson->getRot();
}

TransformComponent& TransformComponent::operator=(const Transform& transform) {
  rot = transform.rot;
  pos = transform.pos;

  return *this;
}

RAMPAGE_END