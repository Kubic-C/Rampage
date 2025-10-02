#include "gui.hpp"

#include "../../event/module.hpp"
#include "../module.hpp"

RAMPAGE_START

int renderGui(EntityWorld& world, float deltaTime) {
  glDisable(GL_DEPTH_TEST);
  world.getContext<tgui::Gui>().draw();
  glEnable(GL_DEPTH_TEST);

  return 0;
}

int observeSdlEvent(Entity sdlEventEntity) {
  EntityWorld& world = sdlEventEntity.world();
  auto& gui = world.getContext<tgui::Gui>();
  auto sdlEvent = sdlEventEntity.get<SDL_Event>();

  gui.handleEvent(*sdlEvent);

  return 0;
}

void loadGuiSystems(IHost& host) {
  EntityWorld& world = host.getWorld();
  Pipeline& pipeline = host.getPipeline();

  pipeline.getGroup<RenderGroup>().attachToStage<RenderGroup::OnGUIRenderStage>(renderGui);

  world.observe<SDL_Event>(world.component<SDL_Event>(), {}, observeSdlEvent);
}

RAMPAGE_END
