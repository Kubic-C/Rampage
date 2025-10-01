#include "health.hpp"

#include "../components/health.hpp"
#include "../module.hpp"

RAMPAGE_START

int healthSystem(EntityWorld& world, float dt) {
  world.beginDefer();
  auto it = world.getWith(world.set<HealthComponent>());
  while (it.hasNext()) {
    Entity e = it.next();
    auto health = e.get<HealthComponent>();

    if (health->health <= 0) {
      e.world().destroy(e);
    }
  }
  world.endDefer();

  return 0;
}

int lifetimeSystem(EntityWorld& world, float dt) {
  world.beginDefer();
  auto it = world.getWith(world.set<LifetimeComponent>());
  while (it.hasNext()) {
    Entity e = it.next();
    auto destroyAfter = e.get<LifetimeComponent>();

    if (destroyAfter->timeLeft < 0) {
      e.world().destroy(e);
    }
    destroyAfter->timeLeft -= dt;
  }
  world.endDefer();

  return 0;
}

void loadHealthSystems(IHost& host) {
  Pipeline& pipeline = host.getPipeline();

  pipeline.getGroup<GameGroup>()
      .attachToStage<GameGroup::PostTickStage>(healthSystem)
      .attachToStage<GameGroup::PostTickStage>(lifetimeSystem);
}

RAMPAGE_END
