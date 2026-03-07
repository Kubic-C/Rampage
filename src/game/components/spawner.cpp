#include "spawner.hpp"

RAMPAGE_START

void SpawnerComponent::serialize(capnp::MessageBuilder& builder, Ref component) {
  auto spawnerBuilder = builder.initRoot<Schema::SpawnerComponent>();
  auto spawner = component.cast<SpawnerComponent>();

  spawnerBuilder.setSpawn(spawner->spawn);
  spawnerBuilder.setSpawnRate(spawner->spawnRate);
  spawnerBuilder.setTimeSinceLastSpawn(spawner->timeSinceLastSpawn);
  spawnerBuilder.setSpawnableRadius(spawner->spawnableRadius);
  spawnerBuilder.setSpawnCount(spawner->spawnCount);
}

void SpawnerComponent::deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component) {
  auto spawnerReader = reader.getRoot<Schema::SpawnerComponent>();
  auto spawner = component.cast<SpawnerComponent>();

  spawner->spawn = id.resolve(spawnerReader.getSpawn());
  spawner->spawnRate = spawnerReader.getSpawnRate();
  spawner->timeSinceLastSpawn = spawnerReader.getTimeSinceLastSpawn();
  spawner->spawnableRadius = spawnerReader.getSpawnableRadius();
  spawner->spawnCount = spawnerReader.getSpawnCount();
}

void SpawnerComponent::fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& jsonValue) {
  auto self = component.cast<SpawnerComponent>();
  auto compJson = jsonValue.as<JSchema::SpawnerComponent>();

  if (compJson->hasSpawn()) {
    self->spawn = loader.getAsset(compJson->getSpawn());
  }
  if (compJson->hasSpawnRate())
    self->spawnRate = compJson->getSpawnRate();
  if (compJson->hasSpawnableRadius())
    self->spawnableRadius = compJson->getSpawnableRadius();
  if (compJson->hasSpawnCount())
    self->spawnCount = compJson->getSpawnCount();
}

RAMPAGE_END