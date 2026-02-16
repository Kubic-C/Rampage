#pragma once

#include "../../common/common.hpp"

RAMPAGE_START

struct SpawnerComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component) {
    auto spawnerBuilder = builder.initRoot<Schema::SpawnerComponent>();
    auto spawner = component.cast<SpawnerComponent>();

    spawnerBuilder.setSpawn(spawner->spawn);
    spawnerBuilder.setSpawnRate(spawner->spawnRate);
    spawnerBuilder.setTimeSinceLastSpawn(spawner->timeSinceLastSpawn);
    spawnerBuilder.setSpawnableRadius(spawner->spawnableRadius);
    spawnerBuilder.setSpawnCount(spawner->spawnCount);
  }

  static void deserialize(capnp::MessageReader& reader, Ref component) {
    auto spawnerReader = reader.getRoot<Schema::SpawnerComponent>();
    auto spawner = component.cast<SpawnerComponent>();

    spawner->spawn = spawnerReader.getSpawn();
    spawner->spawnRate = spawnerReader.getSpawnRate();
    spawner->timeSinceLastSpawn = spawnerReader.getTimeSinceLastSpawn();
    spawner->spawnableRadius = spawnerReader.getSpawnableRadius();
    spawner->spawnCount = spawnerReader.getSpawnCount();
  }

  // The entity to summon
  EntityId spawn;
  // Per second
  float spawnRate = 1.0f;
  float timeSinceLastSpawn = 0.0f;
  // The radius to spawn
  float spawnableRadius = 5.0f;
  // The amount to spawn per summon
  u32 spawnCount = 5;
};

RAMPAGE_END
