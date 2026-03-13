#include "render2D.hpp"
#include "internal.hpp"
#include <bx/bx.h>
#include <bx/math.h>

RAMPAGE_START

class BgfxCallback : public bgfx::CallbackI {
public:
  IHost* host = nullptr;

  void fatal(const char* filePath, uint16_t line, bgfx::Fatal::Enum code, const char* str) override {
    std::cerr << "BGFX FATAL: " << str << std::endl;
    abort();
  }

void traceVargs(const char* filePath, uint16_t line, const char* format, va_list argList) override {
    if (host) {
      host->log(format, argList);
    } else {
      vprintf(format, argList);
    }
  }
    
  void profilerBegin(const char* name, uint32_t abgr, const char* filePath, uint16_t line) override {
    // Stub: profiler region begin
  }
    
  void profilerBeginLiteral(const char* name, uint32_t abgr, const char* filePath, uint16_t line) override {
    // Stub: profiler region begin with literal name
  }
    
  void profilerEnd() override {
    // Stub: profiler region end
  }
    
  uint32_t cacheReadSize(uint64_t id) override {
    // Stub: cache read size
    return 0;
  }
    
  bool cacheRead(uint64_t id, void* data, uint32_t size) override {
    // Stub: cache read
    return false;
  }
    
  void cacheWrite(uint64_t id, const void* data, uint32_t size) override {
    // Stub: cache write
  }
    
  void screenShot(const char* filePath, uint32_t width, uint32_t height, uint32_t pitch, bgfx::TextureFormat::Enum format, const void* data, uint32_t size, bool yflip) override {
    // Stub: screenshot
  }
    
  void captureBegin(uint32_t width, uint32_t height, uint32_t pitch, bgfx::TextureFormat::Enum format, bool yflip) override {
    // Stub: capture begin
  }
    
  void captureEnd() override {
    // Stub: capture end
  }
    
  void captureFrame(const void* data, uint32_t size) override {
    // Stub: capture frame
  }
};

Render2D::Render2D(IHost& host, SDL_Window* window) : m_host(host) {
  static BgfxCallback callback;
  callback.host = &host;
  m_window = window;

  SDL_PropertiesID properties = SDL_GetWindowProperties(window);
  bgfx::Init init;
  init.type = bgfx::RendererType::Vulkan;
  init.vendorId = BGFX_PCI_ID_NONE;
  init.platformData.nwh = SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
  init.resolution.width = 800;
  init.resolution.height = 600;
  init.resolution.reset = BGFX_RESET_VSYNC;
  init.callback = &callback;
  bgfx::init(init);

  bgfx::setDebug(BGFX_DEBUG_TEXT);

  m_renderData = new InternalRender2D();
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

  proj = glm::ortho(-hSize.x, hSize.x, -hSize.y, hSize.y, -100.0f, 0.0f);
  proj = glm::scale(proj, glm::vec3(1.0f, 1.0f, -1.0f));

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

  screenCoords.y = screenSize.y - screenCoords.y;

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

  screenCoords.y = screenSize.y - screenCoords.y;

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
  internal.tileRender.reset();
  internal.shapeRender.reset();
  internal.lightRender.reset();
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
  glm::mat4 view = glm::rowMajor4(getView());
  glm::mat4 proj = glm::rowMajor4(getProj()); 
  bgfx::setViewTransform(RenderPass::World, glm::value_ptr(proj), glm::value_ptr(view));

  int width, height;
  SDL_GetWindowSize(m_window, &width, &height);
  bgfx::setViewRect(RenderPass::World, 0, 0, width, height);
  bgfx::setViewMode(RenderPass::World, bgfx::ViewMode::Sequential);
  bgfx::setViewClear(RenderPass::World, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH);
  
  bgfx::touch(RenderPass::World);
  bgfx::dbgTextPrintf(0, 0, 0x4F, "Frame: %zu", m_frame);

  InternalRender2D& internal = *reinterpret_cast<InternalRender2D*>(m_renderData);
  internal.tileRender.process(m_tileCmds);
  internal.shapeRender.process(m_rectangleCmds);
  internal.shapeRender.process(m_circleCmds);
  internal.shapeRender.process(m_lineCmds);
  internal.shapeRender.process(m_hollowCircleCmds);
  internal.lightRender.process(m_lightCmds);
  internal.tileRender.draw();
  internal.shapeRender.draw(); 
  internal.lightRender.draw();

  bgfx::frame();
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

RAMPAGE_END