#pragma once

#include "../../common/commondef.hpp"
#include "state.hpp"

RAMPAGE_START

class MenuState : public State {
public:
  MenuState(IWorldPtr world) : m_world(world) {
    
  }

  void onEntry() override {
    auto& stateMgr = m_world->getContext<StateManager>();
    stateMgr.disableState("MenuState");
    stateMgr.enableState("PlayState");
  }

  void onLeave() override {

  }

private:
  IWorldPtr m_world;
};

RAMPAGE_END
