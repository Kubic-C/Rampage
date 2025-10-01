#include "host.hpp"

#include "../core/module.hpp"
#include "../event/module.hpp"
#include "../game/module.hpp"
#include "../log/module.hpp"
#include "../render/module.hpp"

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

struct StatsCounterGroup {
  struct StatsCounterStage {};
};

void registerStatsSystems(Pipeline& pipeline) {
  auto& renderGroup = pipeline.getGroup<RenderGroup>();
  renderGroup.attachToStage<RenderGroup::PreRenderStage>([](EntityWorld& world, float dt) {
    auto& stats = world.getContext<AppStats>();

    stats.cumFrames++;

    return 0;
  });

  auto& gameGroup = pipeline.getGroup<GameGroup>();
  gameGroup.attachToStage<GameGroup::TickStage>([](EntityWorld& world, float dt) {
    auto& stats = world.getContext<AppStats>();

    stats.cumTicks++;
    stats.tick++;

    return 0;
  });

  auto& statsCounterGroup =
      pipeline.createGroup<StatsCounterGroup>(1.0f).createStage<StatsCounterGroup::StatsCounterStage>();
  statsCounterGroup.attachToStage<StatsCounterGroup::StatsCounterStage>([](EntityWorld& world, float dt) {
    auto& stats = world.getContext<AppStats>();

    stats.tps = stats.cumTicks;
    stats.fps = stats.cumFrames;
    stats.cumTicks = stats.cumFrames = 0;

    auto& host = world.getHost();
    host.log("FPS/TPS: %4.1f/%4.1f\n", stats.fps, stats.tps);

    return 0;
  });
}

Host::Host() : m_status(Status::Ok), m_world(std::make_unique<EntityWorld>(*this)) {
  addModule<CoreModule>();
  addModule<LogModule>();
  addModule<RenderModule>();
  addModule<EventModule>();
  addModule<GameModule>();

  sortModulesByDependencies();

  for (IStaticModule* module : m_staticModules) {
    int rc = module->onLoad();
    if (rc != 0) {
      log(rc, "Failed to load module: %s\n", module->getName().c_str());
      m_status = Status::CriticalError;
      break;
    }
    log("Loaded: %s\n", module->getName().c_str());
  }

  registerStatsSystems(m_pipeline);
}

int Host::run() {
  while (!m_exit) {
    for (auto module : m_staticModules) {
      module->onUpdate();
    }

    m_pipeline.run(*this);
  }

  m_world.reset();

  return 0;
}

RAMPAGE_END
