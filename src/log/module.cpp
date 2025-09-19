#include "module.hpp"
#include "log.hpp"

RAMPAGE_START

int LogModule::onLoad(IHost& host) {
  logInit();
  host.setLogFuncs(logGeneric, logError);

  return 0;
}

int LogModule::onUnload(IHost& host) {
  host.setLogFuncs(nullptr, nullptr);

  // EntityWorld& world = host.getWorld();
  // auto iter = world.getWith(world.set<TestComponent>());
  // while (iter.hasNext()) {
  //   Entity e = iter.next();
  //   auto test = e.get<TestComponent>();
  //
  //   test->print = true;
  // }

  return 0;
}

int LogModule::onUpdate(IHost& host) {
  EntityWorld& world = host.getWorld();

  // auto iter = world.getWith(world.set<TestComponent>());
  // while (iter.hasNext()) {
  //   Entity e = iter.next();
  //
  //   auto test = e.get<TestComponent>();
  //   if (test->print) {
  //     host.log("LLL %i %f %s\n", test->x, test->y, test->str);
  //     test->print = false;
  //   }
  // }

  return 0;
}

RAMPAGE_END