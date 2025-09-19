#include "module.hpp"

RAMPAGE_START

int CoreModule::onLoad(IHost& host) {
  EntityWorld& world = host.getWorld();

  // world.component<TestComponent>();
  //
  // Entity e = world.create();
  // e.add<TestComponent>();
  // e.get<TestComponent>()->str = "This is anotha str yo!\n";

  return 0;
}

int CoreModule::onUnload(IHost& host) {
  return 0;
}

int CoreModule::onUpdate(IHost& host) {
  return 0;
}

RAMPAGE_END