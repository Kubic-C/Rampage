#pragma once
#include "../common/common.hpp"

/* DO NOT EXPOSE UNDERLYING MODULE IMPLEMENTATION IN THIS FILE. */

RAMPAGE_START

class LogModule final : public IStaticModule {
public:
  std::vector<std::type_index> getDependencies() override {
    return {};
  }

  LogModule(IHost& host) : IStaticModule("LogModule", host) {}

public:
  int onLoad() override;
  int onUpdate() override;
};

RAMPAGE_END
