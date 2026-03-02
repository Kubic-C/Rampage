#include "transform.hpp"

RAMPAGE_START

void TransformComponent::serialize(capnp::MessageBuilder& builder, Ref component) {
  auto transformBuilder = builder.initRoot<Schema::TransformComponent>();
  auto transform = component.cast<TransformComponent>();

  transformBuilder.getPos().setX(transform->pos.x);
  transformBuilder.getPos().setY(transform->pos.y);
  transformBuilder.setRot(transform->rot);
}

void TransformComponent::deserialize(capnp::MessageReader& reader, Ref component) {
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

void TransformComponent::fromJson(Ref component, AssetLoader loader, const json& compJson) {
  auto transform = component.cast<TransformComponent>();

  if(compJson.contains("pos") && compJson["pos"].is_object()) {
    json posJson = compJson["pos"];
    if(posJson.contains("x") && posJson["x"].is_number())
      transform->pos.x = posJson["x"];
    if(posJson.contains("y") && posJson["y"].is_number())
      transform->pos.y = posJson["y"];
  }
  if(compJson.contains("rot") && compJson["rot"].is_number()) {
    transform->rot = compJson["rot"].get<float>();
  }
}

TransformComponent& TransformComponent::operator=(const Transform& transform) {
  rot = transform.rot;
  pos = transform.pos;

  return *this;
}

RAMPAGE_END