#include "module.hpp"

RAMPAGE_START

int GameModule::onLoad() {
  auto& world = m_host->getWorld();

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