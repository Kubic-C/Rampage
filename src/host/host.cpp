#include "host.hpp"

#include "../log/module.hpp"
#include "../core/module.hpp"
#include "../render/module.hpp"
#include "../event/module.hpp"
#include "../game/module.hpp"

RAMPAGE_START

void Host::sortModulesByDependencies() {
  // Make a map that details a child and its parents.
  Map<std::type_index, std::vector<std::type_index>> parentMap;
  for (IStaticModule* module : m_staticModules) {
    std::type_index self = typeid(*module);

    parentMap.emplace(self, module->getDependencies());
  }

  // Use DFS to define a load order using their dependencies.
  std::vector<IStaticModule*> loadOrder;
  Set<std::type_index> visited;
  for (const auto& modType : parentMap | std::views::keys) {
    if (visited.count(modType))
      continue;

    std::vector<std::type_index> stack;
    stack.push_back(modType);
    while (!stack.empty()) {
      std::type_index current = stack.back();
      if (visited.count(current)) {
        stack.pop_back();
        continue;
      }

      bool allDepsVisited = true;
      for (auto& dep : parentMap[current]) {
        if (!visited.count(dep)) {
          stack.push_back(dep);
          allDepsVisited = false;
        }
      }
      if (allDepsVisited) {
        visited.insert(current);
        loadOrder.push_back(m_typeMap[current]);
        stack.pop_back();
      }
    }
  }
  m_staticModules = loadOrder;
}

Host::Host()
  : m_status(Status::Ok) {
  addModule<CoreModule>();
  addModule<LogModule>();
  addModule<RenderModule>();
  addModule<EventModule>();
  addModule<GameModule>();

  sortModulesByDependencies();

  for (IStaticModule* module : m_staticModules) {
    int rc = module->onLoad();
    if (rc !=0) {
      log(rc, "Failed to load module: %s\n", module->getName().c_str());
      m_status = Status::CriticalError;
      break;
    }
    log("Loaded: %s\n", module->getName().c_str());
  }
}

int Host::run() {
  while (!m_exit) {
    for (auto module : m_staticModules) {
      module->onUpdate();
    }

    m_pipeline.run(*this);
  }

  for (auto module : m_staticModules) {
    module->onUnload();
  }

  return 0;
}

RAMPAGE_END