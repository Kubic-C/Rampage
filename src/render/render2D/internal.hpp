#pragma once

#include "commands.hpp"

/* SHOULD NOT BE INCLUDED IN RENDER2D.HPP */

RAMPAGE_START

static constexpr uint64_t DEFAULT_BLEND_STATE = 
  // BGFX_STATE_DEPTH_TEST_ | 
  BGFX_STATE_WRITE_Z |
  BGFX_STATE_WRITE_RGB |
  BGFX_STATE_WRITE_A |
  BGFX_STATE_BLEND_FUNC(
    BGFX_STATE_BLEND_SRC_ALPHA,
    BGFX_STATE_BLEND_INV_SRC_ALPHA
  );

class ShapeRender {
  struct ShapeVertex {
    glm::vec3 pos;
    glm::vec3 color;
  };

public:
  ShapeRender();
  void reset();
  void process(const std::vector<DrawRectangleCmd>& cmds);
  void process(const std::vector<DrawCircleCmd>& cmds);
  void process(const std::vector<DrawLineCmd>& cmds);
  void process(const std::vector<DrawHollowCircleCmd>& cmds);
  void draw();

private:
  bgfx::VertexLayout m_vertexLayout;
  bgfx::TransientVertexBuffer m_vertexBuffer;
  bgfx::ProgramHandle m_program;
  std::vector<ShapeVertex> m_vertices;
  std::vector<ShapeVertex> m_baseCircleMesh;

  void generateCircleMesh(int resolution);
  void addLine(const DrawLineCmd& cmd);
};

class TileRender {
public:
  TileRender();
  void reset();
  void process(const std::vector<DrawTileCmd>& cmds);
  void draw();

private:
};

class LightRender {
public:
  LightRender();
  void reset();
  void process(const std::vector<DrawLightCmd>& cmds);
  void draw();

private:
};

struct InternalRender2D {
  ShapeRender shapeRender;
  TileRender tileRender;
  LightRender lightRender;
};

RAMPAGE_END