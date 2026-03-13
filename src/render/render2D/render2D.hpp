#pragma once

#include "commands.hpp"

RAMPAGE_START

struct Camera {
  glm::vec2 pos = {0, 0};
  float rot = 0;
  float zoom = 1; // screenDim / zoom = viewable worldDim

  // screenDim / zoom = viewable worldDim
  // ppm = screenDim 
};

struct WorldViewRect {
  Vec2 center = Vec2(0);
  Vec2 halfDim = Vec2(std::numeric_limits<float>::max());

  // Check if a point is inside this rectangle
  bool contains(const Vec2& point) const {
    return std::abs(point.x - center.x) <= halfDim.x && std::abs(point.y - center.y) <= halfDim.y;
  }

  // Check if another rectangle (centered at 'point', with half-size 'halfSize') is fully inside this
  // rectangle
  bool intersects(const Vec2& point, const Vec2& halfSize) const {
    return std::abs(point.x - center.x) <= (halfDim.x + halfSize.x) &&
        std::abs(point.y - center.y) <= (halfDim.y + halfSize.y);
  }
};

class Render2D {
  enum RenderPass {
    World = 0,
    Light,
    UI,
    Debug
  };
  
  using InternalRender2DPtr = void*;
  
  Render2D() = delete;
  Render2D(const Render2D& other) = delete;
  Render2D(Render2D&& other) = delete;
  Render2D& operator=(const Render2D& other) = delete;
  Render2D& operator=(Render2D&& other) = delete;
  Render2D& operator=(Render2D& other) = delete;
  Render2D& operator=(Render2D other) = delete;
public:
  Render2D(IHost& host, SDL_Window* window);

  void setCamera(const Camera& camera);
  WorldViewRect getViewRect() const;
  glm::mat4 getProj() const;
  glm::mat4 getView() const;
  glm::mat4 getViewProj() const;
  glm::vec2 getWorldCoords(const glm::ivec2& screenCoords) const;
  glm::vec2 getScreenCoords(const glm::ivec2& worldCoords) const;
  glm::vec2 getPPM() const;

  void clearCmds();
  void begin();
  void submit(const DrawTileCmd& cmd);
  void submit(const DrawRectangleCmd& cmd);
  void submit(const DrawCircleCmd& cmd);
  void submit(const DrawLineCmd& cmd);
  void submit(const DrawHollowCircleCmd& cmd);
  void submit(const DrawLightCmd& cmd);
  void end();
  
  glm::vec2 getWindowSize() const;

  TextureId createTexture(const std::string& path);
  TileTextureId createTileTexture(const std::string& path);
  std::string getTexturePath(TextureId textureId) const;
  std::string getTileTexturePath(TileTextureId textureId) const;

private:
  IHost& m_host;
  SDL_Window* m_window;

  size_t m_frame = 0;
  Camera m_camera;

  std::vector<DrawTileCmd> m_tileCmds;
  std::vector<DrawRectangleCmd> m_rectangleCmds;
  std::vector<DrawCircleCmd> m_circleCmds;
  std::vector<DrawLineCmd> m_lineCmds;
  std::vector<DrawHollowCircleCmd> m_hollowCircleCmds;
  std::vector<DrawLightCmd> m_lightCmds;
  InternalRender2DPtr m_renderData = nullptr;
};

RAMPAGE_END