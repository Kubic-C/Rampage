#include "module.hpp"

#include "../event/module.hpp"
#include "camera.hpp"
#include <SDL3/SDL_system.h>

RAMPAGE_START

int clearWindow(IWorldPtr world, float dt) {
  auto& render = world->getContext<Render2D>();

  auto entity = world->getFirstWith(world->set<CameraInUseTag, TransformComponent>());
  if (entity.exists()) {
    auto camera = entity.get<CameraComponent>();
    auto transform = entity.get<TransformComponent>();

    Camera cameraBegin;
    cameraBegin.pos = transform->pos;
    cameraBegin.rot = camera->rot;
    cameraBegin.zoom = camera->zoom;
    render.setCamera(cameraBegin);
    render.begin();
  }
  
  return 0;
}

int swapBuffers(IWorldPtr world, float dt) {
  auto& render = world->getContext<Render2D>();

  auto entity = world->getFirstWith(world->set<CameraInUseTag, TransformComponent>());
  if (entity.exists())
    render.end();

  return 0;
}

void observeResizeEvent(EntityPtr sdlEventEntity) {
  IWorldPtr world = sdlEventEntity.world();
  auto sdlEvent = sdlEventEntity.get<SDL_Event>();
  auto& render = world->getContext<Render2D>();

  switch (static_cast<Event>(sdlEvent->type)) {
  case Event::WindowResized:
    render.resize({sdlEvent->window.data1, sdlEvent->window.data2});
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
      SDL_CreateWindow(m_host->getTitle().data(), 600, 600,  SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);
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
  world->addContext<Render2D>(*m_host, window);

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

  return 0;
}

int RenderModule::onUpdate() {
  return 0;
}

void RenderModule::enableVsync(bool vsync) {
  SDL_GL_SetSwapInterval((int)vsync);
}

glm::ivec2 RenderModule::getWindowSize() const {
  glm::ivec2 screenSize;
  SDL_GetWindowSize(m_host->getWorld()->getContext<SDL_Window*>(), &screenSize.x, &screenSize.y);

  return screenSize;
}

bool RenderModule::doesCameraExists() const {
  IWorldPtr world = m_host->getWorld();
  EntityPtr cameraEntity = world->getFirstWith(world->set<CameraInUseTag>());

  return cameraEntity.exists();
}

RAMPAGE_END
