#pragma once

#include <random>
#include "../components/spawner.hpp"
#include "../components/transform.hpp"

class SpawnerModule : public Module {
  struct SpawnAt {
    EntityId id;
    Vec2 pos;
  };

  Vec2 getUniformCircularPoint(float radius) {
    static std::uniform_real_distribution<float> unif(-1, 1);

    while (true) {
      float x = unif();
      float y = unif();
      if (x * x + y * y <= 1)
        return Vec2(x, y) * r
    }
  }

  public:
  SpawnerModule(EntityWorld& world) { world.component<SpawnerComponent>(); }

  void run(EntityWorld& world, float deltaTime) {
    EntityIterator it = world.getWith(world.set<TransformComponent, SpawnerComponent>());
    std::vector<SpawnAt> spawned;

    world.beginDefer();
    while (it.hasNext()) {
      Entity e = it.next();

      RefT<TransformComponent> transform = e.get<TransformComponent>();
      RefT<SpawnerComponent> spawner = e.get<SpawnerComponent>();

      spawner->timeSinceLastSpawn -= deltaTime;
      if (spawner->timeSinceLastSpawn <= 0) {
        spawner->timeSinceLastSpawn = spawner->spawnRate;

        for (u32 i = 0; i < spawner->spawnCount; i++) {
          Vec2 point = getUniformCircularPoint(spawner->spawnableRadius);

          spawned.push_back({spawner->spawn, point});
        }
      }
    }
    world.endDefer();

    for (SpawnAt& spawn : spawned) {
      Entity e = world.clone(spawn.id);
      e.get<TransformComponent>()->pos = spawn.pos;
    }
  }

  private:
};
