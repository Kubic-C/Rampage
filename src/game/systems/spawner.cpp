#include "spawner.hpp"

#include <random>

#include "../module.hpp"
#include "../components/spawner.hpp"
#include "../../core/module.hpp"

RAMPAGE_START

struct SpawnAt {
  EntityId id;
  Vec2 pos;
};

Vec2 getUniformCircularPoint(float radius) {
  static std::random_device rd;
  static std::mt19937 generator(rd());
  static std::uniform_real_distribution<float> unif(-1, 1);

  while (true) {
    float x = unif(generator);
    float y = unif(generator);
    if (x * x + y * y <= 1)
      return Vec2(x, y) * radius;
  }
}

int updateSpawners(EntityWorld& world, float dt) {
  EntityIterator it = world.getWith(world.set<TransformComponent, SpawnerComponent>());
  std::vector<SpawnAt> spawned;

  world.beginDefer();
  while (it.hasNext()) {
    Entity e = it.next();

    RefT<TransformComponent> transform = e.get<TransformComponent>();
    RefT<SpawnerComponent> spawner = e.get<SpawnerComponent>();

    spawner->timeSinceLastSpawn -= dt;
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

  return 0;
}

void loadSpawnerSystems(IHost& host) {
  Pipeline& pipeline = host.getPipeline();

  pipeline.getGroup<GameGroup>().attachToStage<GameGroup::TickStage>(updateSpawners);
}

RAMPAGE_END