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

  static void deserialize(capnp::MessageReader& reader, Ref component) {
    auto arrowReader = reader.getRoot<Schema::ArrowComponent>();
    auto arrow = component.cast<ArrowComponent>();

    arrow->dir.x       = arrowReader.getDir().getX();
    arrow->dir.y       = arrowReader.getDir().getY();
    arrow->cost        = arrowReader.getCost();
    arrow->generation  = arrowReader.getGeneration();
    arrow->tileCost    = arrowReader.getTileCost();
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

  PrimaryTargetTag(glz::make_reflectable) {}
};

struct SeekPrimaryTargetTag : SerializableTag {
  SeekPrimaryTargetTag() = default;

  SeekPrimaryTargetTag(glz::make_reflectable) {}
};

RAMPAGE_END

// Specialization of glz::meta for PrimaryTargetTag
template <>
struct glz::meta<rmp::PrimaryTargetTag> {
  using T = rmp::PrimaryTargetTag;
  static constexpr std::string_view tag = "primaryTarget";
};

// Specialization of glz::meta for SeekPrimaryTargetTag
template <>
struct glz::meta<rmp::SeekPrimaryTargetTag> {
  using T = rmp::SeekPrimaryTargetTag;
  static constexpr auto value = object();
};
