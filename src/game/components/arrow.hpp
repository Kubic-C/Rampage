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

  static void fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& jsonValue) {
    auto arrow = component.cast<ArrowComponent>();
    auto compJson = jsonValue.as<JSchema::ArrowComponent>();

    if(compJson->hasDir()) {
      auto dirJson = compJson->getDir();
      if(dirJson.hasX())
        arrow->dir.x = dirJson.getX();
      if(dirJson.hasY())
        arrow->dir.y = dirJson.getY();
    }
    if(compJson->hasCost())
      arrow->cost = compJson->getCost();
    if(compJson->hasGeneration())
      arrow->generation = compJson->getGeneration();
    if(compJson->hasTileCost())
      arrow->tileCost = compJson->getTileCost();
  }

  // Normalized vector
  // points up, down, left, or right
  Vec2 dir = {1.0f, 0.0f};
  float cost = std::numeric_limits<float>::max();
  u32 generation = 0;
  u32 tileCost = 0;
};

struct PrimaryTargetTag : SerializableTag, JsonableTag {
  PrimaryTargetTag() = default;
};

struct SeekPrimaryTargetTag : SerializableTag, JsonableTag {
  SeekPrimaryTargetTag() = default;
};

RAMPAGE_END

