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

void TurretComponent::fromJson(Ref component, AssetLoader loader, const json& compJson) {
  auto self = component.cast<TurretComponent>();

  if (compJson.contains("fireRate") && compJson["fireRate"].is_number())
    self->fireRate = compJson["fireRate"];
  if (compJson.contains("timeSinceLastShot") && compJson["timeSinceLastShot"].is_number())
    self->timeSinceLastShot = compJson["timeSinceLastShot"];
  if (compJson.contains("radius") && compJson["radius"].is_number())
    self->radius = compJson["radius"];
  if (compJson.contains("rot") && compJson["rot"].is_number())
    self->rot = compJson["rot"];
  if (compJson.contains("turnSpeed") && compJson["turnSpeed"].is_number())
    self->turnSpeed = compJson["turnSpeed"];
  if (compJson.contains("shootRange") && compJson["shootRange"].is_number())
    self->shootRange = compJson["shootRange"];
  if (compJson.contains("stopRange") && compJson["stopRange"].is_number())
    self->stopRange = compJson["stopRange"];
  if (compJson.contains("muzzleVelocity") && compJson["muzzleVelocity"].is_number())
    self->muzzleVelocity = compJson["muzzleVelocity"];
  if (compJson.contains("bulletRadius") && compJson["bulletRadius"].is_number())
    self->bulletRadius = compJson["bulletRadius"];
  if (compJson.contains("bulletDamage") && compJson["bulletDamage"].is_number())
    self->bulletDamage = compJson["bulletDamage"];
  if (compJson.contains("bulletHealth") && compJson["bulletHealth"].is_number())
    self->bulletHealth = compJson["bulletHealth"];
  if (compJson.contains("summon") && compJson["summon"].is_string()) {
    self->summon = loader.getAsset(compJson["summon"]).id();
  }
}

RAMPAGE_END