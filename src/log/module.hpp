#pragma once
#include "../common/common.hpp"

/* DO NOT EXPOSE UNDERLYING MODULE IMPLEMENTATION IN THIS FILE. */

RAMPAGE_START

class LogModule final : public IStaticModule {
public:
  std::vector<std::type_index> getDependencies() override {
    return {};
  }

  LogModule()
    : IStaticModule("LogModule") {}

public:
  int onLoad(IHost& host) override;
  int onUnload(IHost& host) override;
  int onUpdate(IHost& host) override;
};

RAMPAGE_END