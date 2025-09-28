#include "health.hpp"

#include "../module.hpp"
#include "../components/health.hpp"

RAMPAGE_START


int healthSystem(EntityWorld& world, float dt) {
  auto it = world.getWith(world.set<HealthComponent>());
  while (it.hasNext()) {
    Entity e = it.next();
    auto health = e.get<HealthComponent>();

    if (health->health <= 0) {
      e.world().destroy(e);
    }
  }

  return 0;
}

int lifetimeSystem(EntityWorld& world, float dt) {
  auto it = world.getWith(world.set<HealthComponent>());
  while (it.hasNext()) {
    Entity e = it.next();
    auto destroyAfter = e.get<LifetimeComponent>();

    if (destroyAfter->timeLeft < 0) {
      e.world().destroy(e);
    }
    destroyAfter->timeLeft -= dt;
  }

  return 0;
}

void loadHealthSystems(IHost& host) {
  Pipeline& pipeline = host.getPipeline();

  pipeline.getGroup<GameGroup>()
    .attachToStage<GameGroup::TickStage>(healthSystem)
    .attachToStage<GameGroup::TickStage>(lifetimeSystem);
}

RAMPAGE_END