#pragma once

#include "../../common/common.hpp"

RAMPAGE_START

struct ArrowComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component) {
    auto arrowBuilder = builder.initRoot<Schema::ArrowComponent>();
    auto arrow = component.cast<ArrowComponent>();

    arrowBuilder.getDir().setX(arrow->dir.x);
    arrowBuilder.getDir().setY(arrow->dir.y);
    arrowBuilder.setCost(arrow->cost);
    arrowBuilder.setGeneration(arrow->generation);
    arrowBuilder.setTileCost(arrow->tileCost);
  }

  static void deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component) {
    auto arrowReader = reader.getRoot<Schema::ArrowComponent>();
    auto arrow = component.cast<ArrowComponent>();

    arrow->dir.x       = arrowReader.getDir().getX();
    arrow->dir.y       = arrowReader.getDir().getY();
    arrow->cost        = arrowReader.getCost();
    arrow->generation  = arrowReader.getGeneration();
    arrow->tileCost    = arrowReader.getTileCost();
  }

  static void fromJson(Ref component, AssetLoader loader, const json& compJson) {
    auto arrow = component.cast<ArrowComponent>();

    if(compJson.contains("dir") && compJson["dir"].is_object()) {
      json dirJson = compJson["dir"];
      if(dirJson.contains("x") && dirJson["x"].is_number())
        arrow->dir.x = dirJson["x"];
      if(dirJson.contains("y") && dirJson["y"].is_number())
        arrow->dir.y = dirJson["y"];
    }
    if(compJson.contains("cost") && compJson["cost"].is_number())
      arrow->cost = compJson["cost"];
    if(compJson.contains("generation") && compJson["generation"].is_number_unsigned())
      arrow->generation = compJson["generation"];
    if(compJson.contains("tileCost") && compJson["tileCost"].is_number_unsigned())
      arrow->tileCost = compJson["tileCost"];
  }

  // Normalized vector
  // points up, down, left, or right
  Vec2 dir = {1.0f, 0.0f};
  float cost = std::numeric_limits<float>::max();
  u32 generation = 0;
  u32 tileCost = 0;
};

struct PrimaryTargetTag : SerializableTag{
  PrimaryTargetTag() = default;
};

struct SeekPrimaryTargetTag : SerializableTag {
  SeekPrimaryTargetTag() = default;
};

RAMPAGE_END

