#pragma once
#include "../log/module.hpp"
#include "contexts.hpp"
#include "transform.hpp"

/* DO NOT EXPOSE UNDERLYING MODULE IMPLEMENTATION IN THIS FILE. */

RAMPAGE_START

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
