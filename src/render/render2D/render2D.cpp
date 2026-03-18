#include "render2D.hpp"
#include "internal/internal.hpp"

RAMPAGE_START

Render2D::Render2D(IHost& host, SDL_Window* window) : m_host(host) {
  m_window = window;

  m_renderData = new InternalRender2D(window, false);
  if(((InternalRender2D*)m_renderData)->getStatus() != Status::Ok) {
    throw std::runtime_error("Internal render returned non-ok status\n");
  }
}

WorldViewRect Render2D::getViewRect() const {
  glm::vec2 screenDim = getWindowSize();

  WorldViewRect viewRect;
  viewRect.center = m_camera.pos;
  Vec2 halfDim = Vec2(screenDim.x * 0.5f, screenDim.y * 0.5f) / m_camera.zoom;
  viewRect.halfDim = halfDim;
  return viewRect;
}

glm::mat4 Render2D::getProj() const {
  glm::mat4 proj = glm::identity<glm::mat4>();
  glm::vec2 hSize = (static_cast<glm::vec2>(getWindowSize()) * 0.5f) / m_camera.zoom;

  proj = glm::ortho(-hSize.x, hSize.x, -hSize.y, hSize.y, -100.0f, 100.0f);
  proj = glm::scale(proj, glm::vec3(1.0f, -1.0f, -1.0f));

  return proj;
}

glm::mat4 Render2D::getView() const {
  glm::mat4 view = glm::identity<glm::mat4>();
  view = glm::rotate(view, -m_camera.rot, glm::vec3(0.0f, 0.0f, 1.0f));
  view = glm::translate(view, glm::vec3(-m_camera.pos, 0.0f));
  
  return view;
}

glm::mat4 Render2D::getViewProj() const {
  return getProj() * getView();
}

glm::vec2 Render2D::getWorldCoords(const glm::ivec2& _screenCoords) const {
  glm::ivec2 screenSize = getWindowSize();
  glm::vec2 screenCoords = {(float)_screenCoords.x, (float)_screenCoords.y};
  glm::mat4 view = getView();
  glm::mat4 proj = getProj();

  glm::vec2 worldCoords = glm::unProject(glm::vec3(screenCoords, 0.0f), view, proj,
                                         glm::vec4(0.0f, 0.0f, (float)screenSize.x, (float)screenSize.y));


  return worldCoords;
}

glm::vec2 Render2D::getScreenCoords(const glm::ivec2& worldCoords) const {
  glm::ivec2 screenSize = getWindowSize();
  glm::mat4 view = getView();
  glm::mat4 proj = getProj();

  glm::vec2 screenCoords = glm::project(glm::vec3(worldCoords, 0.0f), view, proj,
                                        glm::vec4(0.0f, 0.0f, (float)screenSize.x, (float)screenSize.y));

  return screenCoords;
}

glm::vec2 Render2D::getPPM() const {
  return glm::vec2(m_camera.zoom);
}

void Render2D::setCamera(const Camera& camera) {
  m_camera = camera;
}

void Render2D::clearCmds() {
  m_tileCmds.clear();
  m_rectangleCmds.clear();
  m_circleCmds.clear();
  m_lineCmds.clear();
  m_hollowCircleCmds.clear();
  m_lightCmds.clear();
}

void Render2D::begin() {
  InternalRender2D& internal = *reinterpret_cast<InternalRender2D*>(m_renderData);
  internal.tileRender->reset();
  internal.shapeRender->reset();
}

void Render2D::submit(const DrawTileCmd& cmd) {
  m_tileCmds.push_back(cmd);
}

void Render2D::submit(const DrawRectangleCmd& cmd) {
  m_rectangleCmds.push_back(cmd); 
}

void Render2D::submit(const DrawCircleCmd& cmd) {
  m_circleCmds.push_back(cmd);
}

void Render2D::submit(const DrawLightCmd& cmd) {
  m_lightCmds.push_back(cmd); 
}

void Render2D::submit(const DrawLineCmd& cmd) {
  m_lineCmds.push_back(cmd);
}

void Render2D::submit(const DrawHollowCircleCmd& cmd) {
  m_hollowCircleCmds.push_back(cmd);
}

void Render2D::end() {
  InternalRender2D& internal = *reinterpret_cast<InternalRender2D*>(m_renderData);
  internal.tileRender->process(m_tileCmds);
  internal.shapeRender->process(m_rectangleCmds);
  internal.shapeRender->process(m_circleCmds);
  internal.shapeRender->process(m_lineCmds);
  internal.shapeRender->process(m_hollowCircleCmds);
  
  internal.draw(getViewProj());

  ++m_frame;
}

glm::vec2 Render2D::getWindowSize() const {
  int width, height;
  SDL_GetWindowSize(m_window, &width, &height);
  return glm::vec2(width, height);
}

TextureId Render2D::createTexture(const std::string& path) {
  return TextureId::null();
}

TileTextureId Render2D::createTileTexture(const std::string& path) {
  return TileTextureId::null();
}

std::string Render2D::getTexturePath(TextureId textureId) const {
  return "";
}

std::string Render2D::getTileTexturePath(TileTextureId textureId) const {
  return "";
}

void Render2D::resize(const glm::ivec2& size) {
  InternalRender2D& internal = *reinterpret_cast<InternalRender2D*>(m_renderData);

  internal.reset(size);
}

RAMPAGE_END