#include "module.hpp"

#include "../common/box2d/box2dScheduler.hpp"

RAMPAGE_START

int CoreModule::onLoad() {
  auto& world = m_host->getWorld();

  world.addContext<enki::TaskScheduler>();
  enki::TaskScheduler& scheduler = world.getContext<enki::TaskScheduler>();
  scheduler.Initialize();

  b2WorldDef physicsWorldDef = b2DefaultWorldDef();
  physicsWorldDef.gravity = b2Vec2{0};
  physicsWorldDef.workerCount = scheduler.GetNumTaskThreads();
  physicsWorldDef.userTaskContext = &scheduler;
  physicsWorldDef.enqueueTask = enki_b2EnqueueTaskCallback;
  physicsWorldDef.finishTask = enki_b2FinishTaskCallback;
  world.addContext<b2WorldId>(b2CreateWorld(&physicsWorldDef));

  world.addContext<AppStats>();
  world.addContext<DoExit>();

  world.component<TransformComponent>();

  return 0;
}

int CoreModule::onUpdate() {
  auto& world = m_host->getWorld();

  if (world.getContext<DoExit>().exit)
    m_host->exit();

  return 0;
}

RAMPAGE_END
