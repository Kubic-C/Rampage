#pragma once

#include "common.hpp"

RAMPAGE_START

struct TextureType {};
struct TileTextureType {};

using TextureId = StrongId<TextureType, u32>;
using TileTextureId = StrongId<TileTextureType, u16>;


// struct DrawSpriteCmd {
//   u32 texture = 0;
//   glm::vec2 pos = {0, 0};
//   float z = 0;
//   glm::vec2 size = {0, 0};
//   float rot = 0;
// };

struct DrawBaseCmd {
  float z = 0;
};

struct DrawTileCmd : public DrawBaseCmd {
  TileTextureId texture = TileTextureId::null();
  glm::vec2 localOffset = {0, 0};
  glm::vec2 pos = {0, 0};
  glm::ivec2 size = {1, 1};
  float scale = 1.0f;
  float rot = 0;
};

struct DrawRectangleCmd : public DrawBaseCmd {
  glm::vec2 pos = {0, 0}; // the center
  glm::vec2 halfSize = {0, 0};
  float rot = 0;
  glm::vec4 color = {1, 1, 1, 1};
};

struct DrawCircleCmd : public DrawBaseCmd {
  glm::vec2 pos = {0, 0};
  float radius = 0;
  glm::vec4 color = {1, 1, 1, 1};
};

struct DrawLineCmd : public DrawBaseCmd {
  glm::vec2 from = {0, 0};
  glm::vec2 to = {0, 0};
  float thickness = 1;
  glm::vec4 color = {1, 1, 1, 1};
};

struct DrawHollowCircleCmd : public DrawBaseCmd {
  glm::vec2 pos = {0, 0};
  int resolution = 36;
  float radius = 0;
  float thickness = 1;
  glm::vec4 color = {1, 1, 1, 1};
};

struct DrawLightCmd : public DrawBaseCmd {
  glm::vec2 pos = {0, 0};
  glm::vec4 color = {1, 1, 1, 1};

  float strength = 1; // [0, 1] 0 the beam is completely blocked, 1 the beam extends indefinitely
  float startAngle = 0;
  float endAngle = glm::two_pi<float>();
};

RAMPAGE_END