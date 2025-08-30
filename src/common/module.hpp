#pragma once

#include "commondef.hpp"

RAMPAGE_START

struct DynamicModuleMeta {
  std::string name;
  cr_plugin ctx;
  bool isLoaded = false;
};

struct DynamicModuleListMeta {
  std::string tempDir;
  std::vector<DynamicModuleMeta> modules;
};

class Host;

class DynamicModule {
protected:
  virtual int onLoad(Host& host, u32 version) = 0;
  virtual int onUnload(Host& host) = 0;
  virtual int onUpdate(Host& host) = 0;

public:
  DynamicModule(const std::string& name)
    : m_name(name) {}

  virtual ~DynamicModule() = default;

  std::string getName() const {
    return m_name;
  }

  int run(const cr_plugin *ctx, cr_op operation, u32 version);

protected:
  std::string m_name;
};

RAMPAGE_END

template <>
struct glz::meta<rmp::DynamicModuleMeta> {
  using T = rmp::DynamicModuleMeta;
  static constexpr auto value = object("name", &T::name);
};

template <>
struct glz::meta<rmp::DynamicModuleListMeta> {
  using T = rmp::DynamicModuleListMeta;
  static constexpr auto value = object("tempDir", &T::tempDir, "modules", &T::modules);
};

