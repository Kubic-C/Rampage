#include "module.hpp"

#include "../event/module.hpp"
#include "camera.hpp"
#include "stb_image.h"
#include "render.hpp"
#include <SDL3/SDL_system.h>

RAMPAGE_START

int clearWindow(IWorldPtr world, float dt) {
  auto& render = world->getContext<RenderModule>();

  glm::ivec2 screenDim = render.getWindowSize();
  bgfx::setViewRect(0, 0, 0, screenDim.x, screenDim.y);
  bgfx::touch(0); // Ensure view 0 is cleared even if no draw calls are submitted
  
  return 0;
}

int swapBuffers(IWorldPtr world, float dt) {
  auto window = world->getContext<SDL_Window*>();

  bgfx::frame();

  return 0;
}

void observeResizeEvent(EntityPtr sdlEventEntity) {
  IWorldPtr world = sdlEventEntity.world();
  auto sdlEvent = sdlEventEntity.get<SDL_Event>();

  switch (static_cast<Event>(sdlEvent->type)) {
  case Event::WindowResized:
    bgfx::setViewRect(0, 0, 0, sdlEvent->window.data1, sdlEvent->window.data2);
    break;
  default:
    break; 
  }
}

int RenderModule::onLoad() {
  IWorldPtr world = m_host->getWorld();

  /* Window and render setup */
  SDL_Init(SDL_INIT_VIDEO);
  SDL_Window* window =
      SDL_CreateWindow(m_host->getTitle().data(), 800, 600,  SDL_WINDOW_RESIZABLE);
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
  world->addContext<SDL_Window*>(window);

  SDL_PropertiesID properties = SDL_GetWindowProperties(window);
  

  bgfx::Init init;
  init.type = bgfx::RendererType::OpenGL;
  init.vendorId = BGFX_PCI_ID_NONE;
  init.platformData.nwh = SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
  init.resolution.width = 800;
  init.resolution.height = 600;
  init.resolution.reset = BGFX_RESET_VSYNC;
  bgfx::init(init);

  /* Basic state */
#ifndef NDEBUG
  enableOpenglErrorCallback(m_host);
#endif

  if (!TTF_Init()) {
    m_host->log("Failed to init SDL_TTF: %s\n", SDL_GetError());
    return -1;
  }

  bgfx::setState(getRenderStateDefault());
  bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x000000FF, 1.0f, 0);


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

  world->observe<SDL_Event>(world->component<SDL_Event>(), {}, observeResizeEvent);

  enableVsync(false);
  
  world->registerComponent<CameraInUseTag>();
  world->registerComponent<CameraComponent>();
  world->registerComponent<TextureMap3DComponent>();
  world->registerComponent<TextureMapInUseTag>();
  world->registerComponent<VertexLayoutComponent>();
  world->registerComponent<InstanceBufferComponent>();
  world->registerComponent<ShaderComponent>();

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
  IWorldPtr world = m_host->getWorld();
  EntityPtr cameraEntity = world->getFirstWith(world->set<CameraInUseTag>());
  glm::ivec2 screenDim = getWindowSize();

  if(!cameraEntity.exists())
    return glm::identity<glm::mat4>();

  auto camera = cameraEntity.get<CameraComponent>();
  auto transform = cameraEntity.get<TransformComponent>();

  return camera->view(Transform(transform->pos, camera->m_rot), static_cast<glm::vec2>(screenDim), 10);
}

glm::mat4 RenderModule::getViewProj() const {
  return getProj() * getView();
}

glm::ivec2 RenderModule::getWindowSize() const {
  glm::ivec2 screenSize;
  SDL_GetWindowSize(m_host->getWorld()->getContext<SDL_Window*>(), &screenSize.x, &screenSize.y);

  return screenSize;
}

ViewRect RenderModule::getViewRect() const {
  IWorldPtr world = m_host->getWorld();
  EntityPtr cameraEntity = world->getFirstWith(world->set<CameraInUseTag>());
  glm::ivec2 screenDim = getWindowSize();

  if(!doesCameraExists())
    return ViewRect{};

  auto camera = cameraEntity.get<CameraComponent>();
  auto transform = cameraEntity.get<TransformComponent>();

  return camera->getViewRect(Transform(transform->pos, camera->m_rot), static_cast<glm::vec2>(screenDim), 1.0f / camera->m_zoom);
}

bool RenderModule::doesCameraExists() const {
  IWorldPtr world = m_host->getWorld();
  EntityPtr cameraEntity = world->getFirstWith(world->set<CameraInUseTag>());

  return cameraEntity.exists();
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

u64 RenderModule::getRenderStateDefault() {
  return       
      BGFX_STATE_DEPTH_TEST_LESS | 
      BGFX_STATE_WRITE_Z |
      BGFX_STATE_WRITE_RGB |
      BGFX_STATE_WRITE_A |
      BGFX_STATE_BLEND_FUNC(
          BGFX_STATE_BLEND_SRC_ALPHA,
          BGFX_STATE_BLEND_INV_SRC_ALPHA
      );
}

RAMPAGE_END
