#pragma once

#include "player.hpp"

RAMPAGE_START

void PlayerComponent::serialize(capnp::MessageBuilder& builder, Ref component) {
  auto playerBuilder = builder.initRoot<Schema::PlayerComponent>();
  auto player = component.cast<PlayerComponent>();

  playerBuilder.getMouse().setX(player->mouse.x);
  playerBuilder.getMouse().setY(player->mouse.y);
  playerBuilder.setAccel(player->accel);
  playerBuilder.setMaxSpeed(player->maxSpeed);
}

void PlayerComponent::deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component) {
  auto playerReader = reader.getRoot<Schema::PlayerComponent>();
  auto player = component.cast<PlayerComponent>();

  player->mouse.x  = playerReader.getMouse().getX();
  player->mouse.y  = playerReader.getMouse().getY();
  player->accel    = playerReader.getAccel();
  player->maxSpeed = playerReader.getMaxSpeed();
}

void PlayerComponent::fromJson(Ref component, AssetLoader loader, const json& compJson) {
  auto self = component.cast<PlayerComponent>();

  if (compJson.contains("accel") && compJson["accel"].is_number())
    self->accel = compJson["accel"];
  if (compJson.contains("maxSpeed") && compJson["maxSpeed"].is_number())
    self->maxSpeed = compJson["maxSpeed"];
}

RAMPAGE_END