#pragma once
#include "../event/module.hpp"
#include "../render/module.hpp"

/* DO NOT EXPOSE UNDERLYING MODULE IMPLEMENTATION IN THIS FILE. */

RAMPAGE_START

struct GameGroup {
  // A physics steps happens every tick, before the TickStage.

  // When most game systems will be executed
  struct TickStage {};
  // Call systems that remove entities here.
  struct PostTickStage {};
};

class GameModule final : public IStaticModule {
public:
  std::vector<std::type_index> getDependencies() override {
    return {typeid(RenderModule), typeid(EventModule)};
  }

  explicit GameModule(IHost& host) : IStaticModule("GameModule", host) {}

public:
  int onLoad() override;
  int onUpdate() override;
};

RAMPAGE_END
