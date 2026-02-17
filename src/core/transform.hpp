#pragma once

#include <cmath>
#include "../common/ecs/ecs.hpp"
#include "../common/math.hpp"
#include "../common/schema/rampage.capnp.h"

RAMPAGE_START

struct TransformComponent : public Transform {
  static void serialize(capnp::MessageBuilder& builder, Ref component) {
    auto transformBuilder = builder.initRoot<Schema::TransformComponent>();
    auto transform = component.cast<TransformComponent>();

    transformBuilder.getPos().setX(transform->pos.x);
    transformBuilder.getPos().setY(transform->pos.y);
    transformBuilder.setRot(transform->rot);
  }

  static void deserialize(capnp::MessageReader& reader, Ref component) {
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

  TransformComponent& operator=(const Transform& transform) {
    rot = transform.rot;
    pos = transform.pos;

    return *this;
  }
};

RAMPAGE_END
