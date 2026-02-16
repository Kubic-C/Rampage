#pragma once

#include "../../common/common.hpp"

RAMPAGE_START

struct CircleRenderComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component) {
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

  static void deserialize(capnp::MessageReader& reader, Ref component) {
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

  float radius = 0.0f;
  Vec2 offset = Vec2(0);
  float z = 1;
  glm::vec3 color = glm::vec3(1.0f, 0.0f, 0.0f);
};

struct RectangleRenderComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component) {
    auto rectBuilder = builder.initRoot<Schema::RectangleRenderComponent>();
    auto rect = component.cast<RectangleRenderComponent>();

    rectBuilder.setHw(rect->hw);
    rectBuilder.setHh(rect->hh);
    rectBuilder.setZ(rect->z);
    rectBuilder.getColor().setX(rect->color.r);
    rectBuilder.getColor().setY(rect->color.g);
    rectBuilder.getColor().setZ(rect->color.b);
  }

  static void deserialize(capnp::MessageReader& reader, Ref component) {
    auto rectReader = reader.getRoot<Schema::RectangleRenderComponent>();
    auto rect = component.cast<RectangleRenderComponent>();

    rect->hw = rectReader.getHw();
    rect->hh = rectReader.getHh();
    rect->z = rectReader.getZ();
    rect->color.r = rectReader.getColor().getX();
    rect->color.g = rectReader.getColor().getY();
    rect->color.b = rectReader.getColor().getZ();
  }

  float hw = 0.0f;
  float hh = 0.0f;
  float z = 1;
  glm::vec3 color = glm::vec3(1.0f, 0.0f, 0.0f);
};

RAMPAGE_END
