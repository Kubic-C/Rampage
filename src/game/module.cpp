#include "module.hpp"
#include "components/components.hpp"

RAMPAGE_START

int GameModule::onLoad() {
  auto& world = m_host->getWorld();

  registerGameComponents(world);

  return 0;
}

int GameModule::onUnload() {
  return 0;
}

int GameModule::onUpdate() {
  auto& world = m_host->getWorld();

  return 0;
}

RAMPAGE_END