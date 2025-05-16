#pragma once

#include "../utility/math.hpp"

struct ArrowComponent {
  // Normalized vector
  // points up, down, left, or right
  Vec2 dir = { 1.0f, 0.0f };
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

// Specialization of glz::meta for PrimaryTargetTag
template <>
struct glz::meta<PrimaryTargetTag> {
  using T = PrimaryTargetTag;
  static constexpr std::string_view tag = "primaryTarget";
};

// Specialization of glz::meta for SeekPrimaryTargetTag
template <>
struct glz::meta<SeekPrimaryTargetTag> {
  using T = SeekPrimaryTargetTag;
  static constexpr auto value = object();
};