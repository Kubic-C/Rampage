#pragma once

#include "../../common/common.hpp"

RAMPAGE_START

struct SpawnerComponent {
  // The entity to summon
  EntityId spawn;
  // Per second
  float spawnRate;
  float timeSinceLastSpawn;
  // The radius to spawn
  float spawnableRadius;
  // The amount to spawn per summon
  u32 spawnCount;
};

RAMPAGE_END
