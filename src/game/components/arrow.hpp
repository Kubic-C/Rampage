#pragma once

#include "../../common/common.hpp"

RAMPAGE_START

struct ArrowComponent {
  // Normalized vector
  // points up, down, left, or right
  Vec2 dir = {1.0f, 0.0f};
  float cost = std::numeric_limits<float>::max();
  u32 generation = 0;
  u32 tileCost = 0;
};

struct PrimaryTargetTag {
  PrimaryTargetTag() = default;

  PrimaryTargetTag(glz::make_reflectable) {}
};

struct SeekPrimaryTargetTag {
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
