#include "turret.hpp"

RAMPAGE_START

void TurretComponent::serialize(capnp::MessageBuilder& builder, Ref component) {
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

void TurretComponent::deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component) {
  auto turretReader = reader.getRoot<Schema::TurretComponent>();
  auto self = component.cast<TurretComponent>();

  self->summon = id.resolve(turretReader.getSummon());
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

void TurretComponent::fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& jsonValue) {
  auto self = component.cast<TurretComponent>();
  auto compJson = jsonValue.as<JSchema::TurretComponent>();

  if (compJson->hasFireRate())
    self->fireRate = compJson->getFireRate();
  if (compJson->hasTimeSinceLastShot())
    self->timeSinceLastShot = compJson->getTimeSinceLastShot();
  if (compJson->hasRadius())
    self->radius = compJson->getRadius();
  if (compJson->hasRot())
    self->rot = compJson->getRot();
  if (compJson->hasTurnSpeed())
    self->turnSpeed = compJson->getTurnSpeed();
  if (compJson->hasShootRange())
    self->shootRange = compJson->getShootRange();
  if (compJson->hasStopRange())
    self->stopRange = compJson->getStopRange();
  if (compJson->hasMuzzleVelocity())
    self->muzzleVelocity = compJson->getMuzzleVelocity();
  if (compJson->hasBulletRadius())
    self->bulletRadius = compJson->getBulletRadius();
  if (compJson->hasBulletDamage())
    self->bulletDamage = compJson->getBulletDamage();
  if (compJson->hasBulletHealth())
    self->bulletHealth = compJson->getBulletHealth();
  if (compJson->hasSummon()) {
    self->summon = loader.getAsset(compJson->getSummon()).id();
  }
}

RAMPAGE_END