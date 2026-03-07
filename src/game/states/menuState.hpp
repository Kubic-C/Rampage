#pragma once

#include "../../common/commondef.hpp"
#include "state.hpp"

RAMPAGE_START

class MenuState : public State {
  const std::string& menuName = "MainMenu";
  const std::string& exitBtnName = "MainExit";
  const std::string& loadSceneBtnName = "MainLoadScene";
  const std::string& playSceneBtnName = "MainPlayScene";

public:
  MenuState(IWorldPtr world) : m_world(world) {
    tgui::Gui& gui = m_world->getContext<tgui::Gui>();
    m_menu = gui.get(menuName);
    tgui::Button::Ptr exitBtn = gui.get(exitBtnName)->cast<tgui::Button>();
    tgui::Button::Ptr loadScene = gui.get(loadSceneBtnName)->cast<tgui::Button>();
    tgui::Button::Ptr playScene = gui.get(playSceneBtnName)->cast<tgui::Button>();

    exitBtn->onMousePress([=]() { world->getContext<DoExit>().exit = true; });
    playScene->onMousePress([=]() {
      StateManager& stateMgr = world->getContext<StateManager>();
      stateMgr.disableState("MenuState");
      stateMgr.enableState("PlayState");
    });
    loadScene->onMousePress([=]() {
      StateManager& stateMgr = world->getContext<StateManager>();
      stateMgr.disableState("MenuState");
      auto& deserializer = m_world->getDeserializer();
      deserializer.deserializeFromFile(*m_world, "saveFile.rampage");
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
