#pragma once

#include "render/baseRender.hpp"

class GuiRenderModule : public BaseRenderModule {
public:
  GuiRenderModule(EntityWorld& world, size_t priority, tgui::Gui& gui)
    : BaseRenderModule(world, priority), m_gui(gui) {}
 
  void onRender(const glm::mat4& vp) override {
    glDisable(GL_DEPTH_TEST);
    m_gui.draw();
    glEnable(GL_DEPTH_TEST);
  }

private:
  tgui::Gui& m_gui;
};