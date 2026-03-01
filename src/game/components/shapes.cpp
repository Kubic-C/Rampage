#include "shapes.hpp"

RAMPAGE_START

// CircleRenderComponent
void CircleRenderComponent::serialize(capnp::MessageBuilder& builder, Ref component) {
  auto circleBuilder = builder.initRoot<Schema::CircleRenderComponent>();
  auto circle = component.cast<CircleRenderComponent>();

  circleBuilder.setRadius(circle->radius);
  circleBuilder.getOffset().setX(circle->offset.x);
  circleBuilder.getOffset().setY(circle->offset.y);
  circleBuilder.setZ(circle->z);
  circleBuilder.getColor().setX(circle->color.r);
  circleBuilder.getColor().setY(circle->color.g);
  circleBuilder.getColor().setZ(circle->color.b);
}

void CircleRenderComponent::deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component) {
  auto circleReader = reader.getRoot<Schema::CircleRenderComponent>();
  auto circle = component.cast<CircleRenderComponent>();

  circle->radius = circleReader.getRadius();
  circle->offset.x = circleReader.getOffset().getX();
  circle->offset.y = circleReader.getOffset().getY();
  circle->z = circleReader.getZ();
  circle->color.r = circleReader.getColor().getX();
  circle->color.g = circleReader.getColor().getY();
  circle->color.b = circleReader.getColor().getZ();
}

void CircleRenderComponent::fromJson(Ref component, AssetLoader loader, const json& compJson) {
  auto self = component.cast<CircleRenderComponent>();
  if (compJson.contains("radius") && compJson["radius"].is_number()) {
    self->radius = compJson["radius"];
  }
  if (compJson.contains("offset") && compJson["offset"].is_object()) {
    json offsetJson = compJson["offset"];
    if (offsetJson.contains("x") && offsetJson["x"].is_number())
      self->offset.x = offsetJson["x"];
    if (offsetJson.contains("y") && offsetJson["y"].is_number())
      self->offset.y = offsetJson["y"];
  }
  if (compJson.contains("z") && compJson["z"].is_number()) {
    self->z = compJson["z"];
  }
  if (compJson.contains("color") && compJson["color"].is_object()) {
    json colorJson = compJson["color"];
    if (colorJson.contains("r") && colorJson["r"].is_number())
      self->color.r = colorJson["r"];
    if (colorJson.contains("g") && colorJson["g"].is_number())
      self->color.g = colorJson["g"];
    if (colorJson.contains("b") && colorJson["b"].is_number())
      self->color.b = colorJson["b"];
  }
}

// RectangleRenderComponent
void RectangleRenderComponent::serialize(capnp::MessageBuilder& builder, Ref component) {
  auto rectBuilder = builder.initRoot<Schema::RectangleRenderComponent>();
  auto rect = component.cast<RectangleRenderComponent>();

  rectBuilder.setHw(rect->hw);
  rectBuilder.setHh(rect->hh);
  rectBuilder.setZ(rect->z);
  rectBuilder.getColor().setX(rect->color.r);
  rectBuilder.getColor().setY(rect->color.g);
  rectBuilder.getColor().setZ(rect->color.b);
}

void RectangleRenderComponent::deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component) {
  auto rectReader = reader.getRoot<Schema::RectangleRenderComponent>();
  auto rect = component.cast<RectangleRenderComponent>();

  rect->hw = rectReader.getHw();
  rect->hh = rectReader.getHh();
  rect->z = rectReader.getZ();
  rect->color.r = rectReader.getColor().getX();
  rect->color.g = rectReader.getColor().getY();
  rect->color.b = rectReader.getColor().getZ();
}

void RectangleRenderComponent::fromJson(Ref component, AssetLoader loader, const json& compJson) {
  auto self = component.cast<RectangleRenderComponent>();
  if (compJson.contains("hw") && compJson["hw"].is_number()) {
    self->hw = compJson["hw"];
  }
  if (compJson.contains("hh") && compJson["hh"].is_number()) {
    self->hh = compJson["hh"];
  }
  if (compJson.contains("z") && compJson["z"].is_number()) {
    self->z = compJson["z"];
  }
  if (compJson.contains("color") && compJson["color"].is_object()) {
    json colorJson = compJson["color"];
    if (colorJson.contains("r") && colorJson["r"].is_number())
      self->color.r = colorJson["r"];
    if (colorJson.contains("g") && colorJson["g"].is_number())
      self->color.g = colorJson["g"];
    if (colorJson.contains("b") && colorJson["b"].is_number())
      self->color.b = colorJson["b"];
  }
}

RAMPAGE_END