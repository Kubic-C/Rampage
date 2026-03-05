#pragma once

#include "body.hpp"

RAMPAGE_START

struct TurretComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component);
  static void deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component);
  static void fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& compJson);

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
