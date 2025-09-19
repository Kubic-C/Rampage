#pragma once
#include "../log/module.hpp"

/* DO NOT EXPOSE UNDERLYING MODULE IMPLEMENTATION IN THIS FILE. */

RAMPAGE_START

class CoreModule final : public IStaticModule {
public:
  virtual std::vector<std::type_index> getDependencies() override {
    return { typeid(LogModule) };
  }

  CoreModule()
    : IStaticModule("CoreModule") {}

public:
  int onLoad(IHost& host) override;
  int onUnload(IHost& host) override;
  int onUpdate(IHost& host) override;
};

RAMPAGE_END