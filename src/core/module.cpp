#include "module.hpp"

RAMPAGE_START

int CoreModule::onLoad() {
  auto& world = m_host->getWorld();

  world.addContext<AppStats>();
  world.addContext<DoExit>();

  world.component<TransformComponent>();

  return 0;
}

int CoreModule::onUnload() {
  return 0;
}

int CoreModule::onUpdate() {
  auto& world = m_host->getWorld();

  if (world.getContext<DoExit>().exit)
    m_host->exit();

  return 0;
}

RAMPAGE_END