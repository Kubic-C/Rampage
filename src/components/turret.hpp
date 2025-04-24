#pragma once

#include "../utility/ecs.hpp"

enum PhysicsCategories {
  Friendly = 0x01,
  Enemy = 0x02,
  Static = 0x04,
  All = 0xFFFF
};

struct TurretComponent {
  EntityId summon = 0;
  float fireRate = 1; // Per second
  float timeSinceLastShot = 0;
  float radius = 2.0f; // search radius

  float rot = 0; // When 0, the turret is facing right
  float turnSpeed = 0.1f;
  float shootRange = 0.04f; // + or -
  float stopRange = 0.02f;

  float muzzleVelocity = 10.0f;
  float bulletRadius = 0.25f;
};