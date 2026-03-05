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

void CircleRenderComponent::fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& jsonValue) {
  auto self = component.cast<CircleRenderComponent>();
  auto compJson = jsonValue.as<JSchema::CircleRenderComponent>();

  if (compJson->hasRadius())
    self->radius = compJson->getRadius();
  if (compJson->hasOffset()) {
    auto offsetJson = compJson->getOffset();
    if (offsetJson.hasX())
      self->offset.x = offsetJson.getX();
    if (offsetJson.hasY())
      self->offset.y = offsetJson.getY();
  }
  if (compJson->hasZ())
    self->z = compJson->getZ();
  if (compJson->hasColor()) {
    auto colorJson = compJson->getColor();
    if (colorJson.hasR())
      self->color.r = colorJson.getR();
    if (colorJson.hasG())
      self->color.g = colorJson.getG();
    if (colorJson.hasB())
      self->color.b = colorJson.getB();
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

void RectangleRenderComponent::fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& jsonValue) {
  auto self = component.cast<RectangleRenderComponent>();
  auto compJson = jsonValue.as<JSchema::RectangleRenderComponent>();

  if (compJson->hasHw())
    self->hw = compJson->getHw();
  if (compJson->hasHh())
    self->hh = compJson->getHh();
  if (compJson->hasZ())
    self->z = compJson->getZ();
  if (compJson->hasColor()) {
    auto colorJson = compJson->getColor();
    if (colorJson.hasR())
      self->color.r = colorJson.getR();
    if (colorJson.hasG())
      self->color.g = colorJson.getG();
    if (colorJson.hasB())
      self->color.b = colorJson.getB();
  }
}

RAMPAGE_END