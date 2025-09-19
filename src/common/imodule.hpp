#pragma once

#include "commondef.hpp"

RAMPAGE_START

class IHost;

class IStaticModule {
public:
  explicit IStaticModule(const std::string& name)
    : m_name(name) {}

  virtual ~IStaticModule() = default;

  std::string getName() const {
    return m_name;
  }

public:
  virtual int onLoad(IHost& host) = 0;
  virtual int onUnload(IHost& host) = 0;
  virtual int onUpdate(IHost& host) = 0;
  virtual std::vector<std::type_index> getDependencies() = 0;

protected:
  std::string m_name;
};

RAMPAGE_END