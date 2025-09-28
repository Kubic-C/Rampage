#include "guiRender.hpp"

#include "../module.hpp"

RAMPAGE_START

int renderGui(EntityWorld& world, float deltaTime)  {
  glDisable(GL_DEPTH_TEST);
  world.getContext<tgui::Gui>().draw();
  glEnable(GL_DEPTH_TEST);

  return 0;
}

void loadGuiRender(IHost& host) {
  Pipeline& pipeline = host.getPipeline();

  pipeline.getGroup<RenderGroup>()
    .attachToStage<RenderGroup::OnGUIRenderStage>(renderGui);
}

RAMPAGE_END