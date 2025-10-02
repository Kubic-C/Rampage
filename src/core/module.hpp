#pragma once
#include "../log/module.hpp"
#include "contexts.hpp"
#include "transform.hpp"

/* DO NOT EXPOSE UNDERLYING MODULE IMPLEMENTATION IN THIS FILE. */

RAMPAGE_START

struct GameGroup {
  // A physics steps happens every tick, before the TickStage but after the PreTickStage.

  // Window events handled here.
  struct PreTickStage {};
  // *Physics Stages*
  // When most game systems will be executed
  struct TickStage {};
  // Call systems that remove entities here.
  struct PostTickStage {};
};

class CoreModule final : public IStaticModule {
public:
  std::vector<std::type_index> getDependencies() override {
    return {typeid(LogModule)};
  }

  explicit CoreModule(IHost& host) : IStaticModule("CoreModule", host) {}

public:
  int onLoad() override;
  int onUpdate() override;
};

RAMPAGE_END
