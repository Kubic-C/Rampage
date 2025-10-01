#include "module.hpp"

#include "camera.hpp"
#include "opengl/opengl.hpp"
#include "stb_image.h"

RAMPAGE_START

static constexpr int MinimumMajorGLVersion = 4;
static constexpr int MinimumMinorGLVersion = 0;
static void setNecessaryGLAttributes() {
#ifndef NDEBUG
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, MinimumMajorGLVersion);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, MinimumMinorGLVersion);
}

int clearWindow(EntityWorld& world, float dt) {
  auto render = world.getContext<RenderModule>();
  auto size = render.getWindowSize();

  glViewport(0, 0, size.x, size.y);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  return 0;
}

int swapBuffers(EntityWorld& world, float dt) {
  auto window = world.getContext<SDL_Window*>();

  return SDL_GL_SwapWindow(window);
}

int RenderModule::onLoad() {
  EntityWorld& world = m_host->getWorld();

  /* Window and render setup */
  SDL_Init(SDL_INIT_VIDEO);
  setNecessaryGLAttributes();
  SDL_Window* window =
      SDL_CreateWindow(m_host->getTitle().data(), 800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
  if (!window) {
    m_host->log(1, "Failed to create window. SDL_GetError(): %s\n", SDL_GetError());
    return -1;
  }
  {
    int x, y, channels;
    u8* data = stbi_load("./res/appIcon.png", &x, &y, &channels, STBI_rgb_alpha);
    if (data != nullptr) {
      SDL_SetWindowIcon(window, SDL_CreateSurfaceFrom(x, y, SDL_PIXELFORMAT_RGBA32, data, x * 4));
    }
  }
  world.addContext<SDL_Window*>(window);

  /* OpengGL Context */
  world.addContext<SDL_GLContext>(SDL_GL_CreateContext(window));
  if (!world.getContext<SDL_GLContext>()) {
    m_host->log(-2, "Failed to create context. SDL_GetError(): %s\n", SDL_GetError());
    return -2;
  }

  if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress)) {
    m_host->log(-3, "Failed to load OpenGL Context: %i %i", MinimumMajorGLVersion, MinimumMinorGLVersion);
    return -3;
  }

  /* Basic state */
#ifndef NDEBUG
  enableOpenglErrorCallback(m_host);
#endif

  if (!TTF_Init()) {
    m_host->log("Failed to init SDL_TTF: %s\n", SDL_GetError());
    return -1;
  }

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  // glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA,
  // GL_DST_ALPHA);

  glEnable(GL_DEPTH_TEST);
  glClearColor(0.2, 1, 0, 0);

  auto& pipeline = m_host->getPipeline();
  Pipeline::Group& group = pipeline.createGroup<RenderGroup>(9999999999.0f)
                               .createStage<RenderGroup::PreRenderStage>()
                               .createStage<RenderGroup::ClearWindowStage>()
                               .createStage<RenderGroup::OnRenderStage>()
                               .createStage<RenderGroup::OnGUIRenderStage>()
                               .createStage<RenderGroup::SwapBuffersStage>()
                               .createStage<RenderGroup::PostRenderStage>();

  group.attachToStage<RenderGroup::ClearWindowStage>(clearWindow);
  group.attachToStage<RenderGroup::SwapBuffersStage>(swapBuffers);

  enableVsync(false);

  return 0;
}

int RenderModule::onUpdate() {
  return 0;
}

void RenderModule::enableVsync(bool vsync) {
  SDL_GL_SetSwapInterval((int)vsync);
}

glm::mat4 RenderModule::getProj() const {
  const glm::vec2 hsize = static_cast<glm::vec2>(getWindowSize()) * 0.5f;
  return glm::ortho<float>(-hsize.x, hsize.x, -hsize.y, hsize.y);
}

glm::mat4 RenderModule::getView() const {
  EntityWorld& world = m_host->getWorld();
  Entity cameraEntity = world.getFirstWith(world.set<CameraInUseTag>());
  glm::ivec2 screenDim = getWindowSize();

  auto camera = cameraEntity.get<CameraComponent>();
  auto transform = cameraEntity.get<TransformComponent>();

  return camera->view(Transform(transform->pos, camera->m_rot), static_cast<glm::vec2>(screenDim));
}

glm::mat4 RenderModule::getViewProj() const {
  return getProj() * getView();
}

glm::ivec2 RenderModule::getWindowSize() const {
  glm::ivec2 screenSize;
  SDL_GetWindowSize(m_host->getWorld().getContext<SDL_Window*>(), &screenSize.x, &screenSize.y);

  return screenSize;
}

glm::vec2 RenderModule::getWorldCoords(const glm::ivec2& _screenCoords) const {
  glm::ivec2 screenSize = getWindowSize();
  glm::vec2 screenCoords = {(float)_screenCoords.x, (float)_screenCoords.y};
  glm::mat4 view = getView();
  glm::mat4 proj = getProj();

  screenCoords.y = screenSize.y - screenCoords.y;

  glm::vec2 worldCoords = glm::unProject(glm::vec3(screenCoords, -1.0f), view, proj,
                                         glm::vec4(0.0f, 0.0f, (float)screenSize.x, (float)screenSize.y));

  return worldCoords;
}

glm::vec2 RenderModule::getScreenCoords(const glm::ivec2& worldCoords) const {
  glm::ivec2 screenSize = getWindowSize();
  glm::mat4 view = getView();
  glm::mat4 proj = getProj();

  glm::vec2 screenCoords = glm::project(glm::vec3(screenCoords, -1.0f), view, proj,
                                        glm::vec4(0.0f, 0.0f, (float)screenSize.x, (float)screenSize.y));

  screenCoords.y = screenSize.y - screenCoords.y;

  return screenCoords;
}

RAMPAGE_END
