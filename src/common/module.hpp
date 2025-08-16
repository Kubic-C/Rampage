#pragma once
#include <vector>
#include <string>
#include <glaze/glaze.hpp>
#include <cr.h>

struct Module {
  std::string name;
  cr_plugin ctx;
  bool isLoaded = false;
};

struct ModuleList {
  std::string tempDir;
  std::vector<Module> modules;
};

template <>
struct glz::meta<Module> {
  using T = Module;
  static constexpr auto value = object("name", &T::name);
};

template <>
struct glz::meta<ModuleList> {
  using T = ModuleList;
  static constexpr auto value = object("tempDir", &T::tempDir, "modules", &T::modules);
};

