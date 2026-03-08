#pragma once

#include "state.hpp"
#include "../core/module.hpp"

RAMPAGE_START

class LostState : public State {
  const std::string& menuName = "LostMenu";
  const std::string& exitBtnName = "LostExit";
  const std::string& exitToMenuBtnName = "LostExitToMenu";

public:
  LostState(IWorldPtr world) : m_world(world) {
    tgui::Gui& gui = m_world->getContext<tgui::Gui>();
    m_menu = gui.get(menuName);
    tgui::Button::Ptr exitBtn = gui.get(exitBtnName)->cast<tgui::Button>();
    tgui::Button::Ptr exitToMenuBtn = gui.get(exitToMenuBtnName)->cast<tgui::Button>();

    exitBtn->onMousePress([=]() { world->getContext<DoExit>().exit = true; });
    exitToMenuBtn->onMousePress([=]() {
      StateManager& stateMgr = world->getContext<StateManager>();
      stateMgr.disableState("LostState");
      stateMgr.enableState("MenuState");
     });
  }

  void onEntry() override {
    m_menu->setEnabled(true);
    m_menu->setVisible(true);
  }

  void onLeave() override {
    m_menu->setEnabled(false);
    m_menu->setVisible(false);
  }

private:
  tgui::Widget::Ptr m_menu;
  IWorldPtr m_world;
};

RAMPAGE_END