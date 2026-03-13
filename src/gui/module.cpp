#include "module.hpp"

RAMPAGE_START

int renderGui(IWorldPtr world, float deltaTime) {
  auto& gui = world->getContext<gui::Gui::Ptr>();

  gui->update();
  gui->render();

  return 0;
}

int observeSdlEvent(EntityPtr sdlEventEntity) {
  IWorldPtr world = sdlEventEntity.world();
  auto sdlEvent = sdlEventEntity.get<SDL_Event>();

  auto& gui = world->getContext<gui::Gui::Ptr>();
  SDL_Event event = *sdlEvent;
  gui->poll(event);

  return 0;
}

int GuiModule::onLoad() {
  IWorldPtr world = m_host->getWorld();
  Pipeline& pipeline = m_host->getPipeline();

  world->addContext<gui::Gui::Ptr>(gui::Gui::create(world->getContext<Render2D>()));

  pipeline.getGroup<RenderGroup>().attachToStage<RenderGroup::OnGUIRenderStage>(renderGui);
  world->observe<SDL_Event>(world->component<SDL_Event>(), {}, observeSdlEvent);

  return 0;
}

int GuiModule::onUpdate() {
  return 0;
}

RAMPAGE_END
