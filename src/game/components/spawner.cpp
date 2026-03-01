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

  spawner->spawn = spawnerReader.getSpawn();
  spawner->spawnRate = spawnerReader.getSpawnRate();
  spawner->timeSinceLastSpawn = spawnerReader.getTimeSinceLastSpawn();
  spawner->spawnableRadius = spawnerReader.getSpawnableRadius();
  spawner->spawnCount = spawnerReader.getSpawnCount();
}

void SpawnerComponent::fromJson(Ref component, AssetLoader loader, const json& compJson) {
  auto self = component.cast<SpawnerComponent>();

  if (compJson.contains("spawn") && compJson["spawn"].is_string()) {
    std::string spawnName = compJson["spawn"];
    self->spawn = loader.getAsset(spawnName);
  }
  if (compJson.contains("spawnRate") && compJson["spawnRate"].is_number())
    self->spawnRate = compJson["spawnRate"];
  if (compJson.contains("spawnableRadius") && compJson["spawnableRadius"].is_number())
    self->spawnableRadius = compJson["spawnableRadius"];
  if (compJson.contains("spawnCount") && compJson["spawnCount"].is_number_unsigned())
    self->spawnCount = compJson["spawnCount"];
}

RAMPAGE_END