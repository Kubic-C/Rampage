#pragma once

#include "../utility/math.hpp"
#include "../schema/rampage.capnp.h"

struct TransformComponent : public Transform {
  static void serialize(capnp::MessageBuilder& builder, Ref component) {
    auto transformBuilder = builder.initRoot<Schema::Transform>();
    auto transform = component.cast<TransformComponent>();

    std::cout << "WRITING AS: " << "Rot:" << transform->rot << ". Pos: " << transform->pos.x << "/" << transform->pos.y << std::endl;

    transformBuilder.getPos().setX(transform->pos.x);
    transformBuilder.getPos().setY(transform->pos.y);
    transformBuilder.setRot(transform->rot);
  }

  static void deserialize(capnp::MessageReader& reader, Ref component) {
    auto transformReader = reader.getRoot<Schema::Transform>();
    auto transform = component.cast<TransformComponent>();

    std::cout << "READING AS: " << "Rot:" << transformReader.getRot()
              << ". Pos: " << transformReader.getPos().getX() << "/" << transformReader.getPos().getY()
              << std::endl;

    transform->rot = transformReader.getRot();
    transform->pos.x = transformReader.getPos().getX();
    transform->pos.y = transformReader.getPos().getY();
  }

  TransformComponent& operator=(const Transform& transform) {
    rot = transform.rot;
    pos = transform.pos;

    return *this;
  }
};

