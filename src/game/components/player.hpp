#pragma once

#include "../../common/common.hpp"

RAMPAGE_START

struct PlayerComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component) {
    auto playerBuilder = builder.initRoot<Schema::PlayerComponent>();
    auto player = component.cast<PlayerComponent>();

    playerBuilder.getMouse().setX(player->mouse.x);
    playerBuilder.getMouse().setY(player->mouse.y);
    playerBuilder.setAccel(player->accel);
    playerBuilder.setMaxSpeed(player->maxSpeed);
  }

  static void deserialize(capnp::MessageReader& reader, Ref component) {
    auto playerReader = reader.getRoot<Schema::PlayerComponent>();
    auto player = component.cast<PlayerComponent>();

    player->mouse.x  = playerReader.getMouse().getX();
    player->mouse.y  = playerReader.getMouse().getY();
    player->accel    = playerReader.getAccel();
    player->maxSpeed = playerReader.getMaxSpeed();
  }

  Vec2 mouse = {0, 0};
  float accel = 1.0f;
  float maxSpeed = 5.0f;
};

RAMPAGE_END
