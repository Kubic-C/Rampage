#pragma once
#include "../core/module.hpp"
#include "../event/module.hpp"
#include "gui/gui.hpp"

RAMPAGE_START

class GuiModule final : public IStaticModule {
public:
  std::vector<std::type_index> getDependencies() override {
    return {typeid(EventModule), typeid(CoreModule)};
  }

  explicit GuiModule(IHost& host) : IStaticModule("GuiModule", host) {}

public:
  int onLoad() override;
  int onUpdate() override;
};

RAMPAGE_END
