#pragma once

#include "body.hpp"

RAMPAGE_START

struct TurretComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component) {
    auto turretBuilder = builder.initRoot<Schema::TurretComponent>();
    auto self = component.cast<TurretComponent>();

    turretBuilder.setSummon(self->summon);
    turretBuilder.setFireRate(self->fireRate);
    turretBuilder.setTimeSinceLastShot(self->timeSinceLastShot);
    turretBuilder.setRadius(self->radius);
    turretBuilder.setRot(self->rot);
    turretBuilder.setTurnSpeed(self->turnSpeed);
    turretBuilder.setShootRange(self->shootRange);
    turretBuilder.setStopRange(self->stopRange);
    turretBuilder.setMuzzleVelocity(self->muzzleVelocity);
    turretBuilder.setBulletRadius(self->bulletRadius);
    turretBuilder.setBulletDamage(self->bulletDamage);
    turretBuilder.setBulletHealth(self->bulletHealth);
  }

  static void deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component) {
    auto turretReader = reader.getRoot<Schema::TurretComponent>();
    auto self = component.cast<TurretComponent>();

    self->summon = turretReader.getSummon();
    self->fireRate = turretReader.getFireRate();
    self->timeSinceLastShot = turretReader.getTimeSinceLastShot();
    self->radius = turretReader.getRadius();
    self->rot = turretReader.getRot();
    self->turnSpeed = turretReader.getTurnSpeed();
    self->shootRange = turretReader.getShootRange();
    self->stopRange = turretReader.getStopRange();
    self->muzzleVelocity = turretReader.getMuzzleVelocity();
    self->bulletRadius = turretReader.getBulletRadius();
    self->bulletDamage = turretReader.getBulletDamage();
    self->bulletHealth = turretReader.getBulletHealth();
  }

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
