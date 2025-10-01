#pragma once

#include "body.hpp"

RAMPAGE_START

struct TurretComponent {
  EntityId summon = 0; /* Currently unused, will be used later. */
  float fireRate = 1; // Per second
  float timeSinceLastShot = 0;
  float radius = 2.0f; // search radius

  float rot = 0; // When 0, the turret is facing right
  float turnSpeed = 0.1f;
  float shootRange = 0.04f; // + or -
  float stopRange = 0.02f;

  float muzzleVelocity = 10.0f;
  float bulletRadius = 0.25f;
  float bulletDamage = 10.0f;
  float bulletHealth = 1.0f;
};

RAMPAGE_END

template <>
struct glz::meta<rmp::TurretComponent> {
  using T = rmp::TurretComponent;
  static constexpr auto value =
      object("summon", &T::summon, "fireRate", &T::fireRate, "timeSinceLastShot", &T::timeSinceLastShot,
             "radius", &T::radius, "rot", &T::rot, "turnSpeed", &T::turnSpeed, "shootRange", &T::shootRange,
             "stopRange", &T::stopRange, "muzzleVelocity", &T::muzzleVelocity, "bulletRadius",
             &T::bulletRadius, "bulletDamage", &T::bulletDamage, "bulletHealth", &T::bulletHealth);
};
