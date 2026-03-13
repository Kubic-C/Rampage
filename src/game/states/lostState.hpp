#pragma once

#include "state.hpp"
#include "../core/module.hpp"

RAMPAGE_START

class LostState : public State {
public:
  LostState(IWorldPtr world) : m_world(world) {

  }

  void onEntry() override {
  }

  void onLeave() override {
  }

private:
  IWorldPtr m_world;
};

RAMPAGE_END