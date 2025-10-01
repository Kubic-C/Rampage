#pragma once

#include "commondef.hpp"

RAMPAGE_START

class IHost;

class IStaticModule {
public:
  explicit IStaticModule(const std::string& name, IHost& host) : m_name(name), m_host(&host) {}

  virtual ~IStaticModule() = default;

  std::string getName() const {
    return m_name;
  }

public:
  virtual int onLoad() = 0;
  virtual int onUpdate() = 0;
  virtual std::vector<std::type_index> getDependencies() = 0;

protected:
  std::string m_name;
  IHost* m_host;
};

RAMPAGE_END
