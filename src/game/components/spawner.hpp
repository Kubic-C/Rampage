#pragma once

#include "../../common/common.hpp"

RAMPAGE_START

struct SpawnerComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component);
  static void deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component);
  static void fromJson(Ref component, AssetLoader loader, const json& compJson);

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
