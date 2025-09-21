#include "module.hpp"
#include "log.hpp"

RAMPAGE_START

int LogModule::onLoad() {
  logInit();
  m_host->setLogFuncs(logGeneric, logError);

  return 0;
}

int LogModule::onUnload() {
  m_host->setLogFuncs(nullptr, nullptr);

  return 0;
}

int LogModule::onUpdate() {
  EntityWorld& world = m_host->getWorld();

  return 0;
}

RAMPAGE_END