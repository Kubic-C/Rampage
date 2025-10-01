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

int updateGui(EntityWorld& world, float deltaTime) {
  auto& eventMgr = world.getContext<EventModule>();
  auto& gui = world.getContext<tgui::Gui>();

  const std::vector<SDL_Event>& polledEvents = eventMgr.getPolledEvents();
  for (const SDL_Event& event : polledEvents) {
    gui.handleEvent(event);
  }

  eventMgr.clearPolledEvents();

  return 0;
}

void loadGuiSystems(IHost& host) {
  Pipeline& pipeline = host.getPipeline();

  pipeline.getGroup<RenderGroup>().attachToStage<RenderGroup::OnGUIRenderStage>(renderGui);

  pipeline.getGroup<GameGroup>().attachToStage<GameGroup::TickStage>(updateGui);
}

RAMPAGE_END
