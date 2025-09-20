#pragma once
#include "../render/module.hpp"
#include "../event/module.hpp"

/* DO NOT EXPOSE UNDERLYING MODULE IMPLEMENTATION IN THIS FILE. */

RAMPAGE_START

class GameModule final : public IStaticModule {
public:
  std::vector<std::type_index> getDependencies() override {
    return { typeid(RenderModule), typeid(EventModule) };
  }

  explicit GameModule(IHost& host)
    : IStaticModule("GameModule", host) {}

public:
  int onLoad() override;
  int onUnload() override;
  int onUpdate() override;
};

RAMPAGE_END