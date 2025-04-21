#pragma once

#include "utility/ecs.hpp"

struct ContactDamageComponent {
  float damage = 10.0f;
};

struct LifetimeComponent {
  float timeLeft = 1.0f;
};

struct HealthComponent {
  float health = 5.0f;
};

class HealthModule : public Module {
public:
  HealthModule(EntityWorld& world)   
    : m_shouldDieSys(world.system(world.set<LifetimeComponent>(), &lifetimeSystem)),
      m_healthSys(world.system(world.set<HealthComponent>(), &healthSystem)) {
    world.component<ContactDamageComponent>();
  }

  static void healthSystem(Entity e, float dt) {
    HealthComponent& health = e.get<HealthComponent>();

    if (health.health <= 0) {
      e.world().destroy(e);
    }
  }

  static void lifetimeSystem(Entity e, float dt) {
    LifetimeComponent& destroyAfter = e.get<LifetimeComponent>();

    if (destroyAfter.timeLeft < 0) {
      e.world().destroy(e);
    }
    destroyAfter.timeLeft -= dt;
  }

  void run(EntityWorld& world, float dt) override {
    m_shouldDieSys.run(dt);
    m_healthSys.run(dt);
  }

private:
  System m_shouldDieSys;
  System m_healthSys;
};