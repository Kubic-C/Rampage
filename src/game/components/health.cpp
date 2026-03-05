#include "health.hpp"

RAMPAGE_START

// ContactDamageComponent
void ContactDamageComponent::serialize(capnp::MessageBuilder& builder, Ref component) {
  auto contactDmgBuilder = builder.initRoot<Schema::ContactDamageComponent>();
  auto self = component.cast<ContactDamageComponent>();
  contactDmgBuilder.setDamage(self->damage);
}

void ContactDamageComponent::deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component) {
  auto contactDmgReader = reader.getRoot<Schema::ContactDamageComponent>();
  auto self = component.cast<ContactDamageComponent>();
  self->damage = contactDmgReader.getDamage();
}

void ContactDamageComponent::fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& jsonValue) {
  auto self = component.cast<ContactDamageComponent>();
  auto compJson = jsonValue.as<JSchema::ContactDamageComponent>();

  if(compJson->hasDamage())
    self->damage = compJson->getDamage();
}

// BulletDamageComponent
void BulletDamageComponent::serialize(capnp::MessageBuilder& builder, Ref component) {
  auto bulletDmgBuilder = builder.initRoot<Schema::BulletDamageComponent>();
  auto self = component.cast<BulletDamageComponent>();
  bulletDmgBuilder.setDamage(self->damage);
}

void BulletDamageComponent::deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component) {
  auto bulletDmgReader = reader.getRoot<Schema::BulletDamageComponent>();
  auto self = component.cast<BulletDamageComponent>();
  self->damage = bulletDmgReader.getDamage();
}

void BulletDamageComponent::fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& jsonValue) {
  auto self = component.cast<BulletDamageComponent>();
  auto compJson = jsonValue.as<JSchema::BulletDamageComponent>();

  if(compJson->hasDamage())
    self->damage = compJson->getDamage();
}

// LifetimeComponent
void LifetimeComponent::serialize(capnp::MessageBuilder& builder, Ref component) {
  auto lifetimeBuilder = builder.initRoot<Schema::LifetimeComponent>();
  auto self = component.cast<LifetimeComponent>();
  lifetimeBuilder.setTimeLeft(self->timeLeft);
}

void LifetimeComponent::deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component) {
  auto lifetimeReader = reader.getRoot<Schema::LifetimeComponent>();
  auto self = component.cast<LifetimeComponent>();
  self->timeLeft = lifetimeReader.getTimeLeft();
}

void LifetimeComponent::fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& jsonValue) {
  auto self = component.cast<LifetimeComponent>();
  auto compJson = jsonValue.as<JSchema::LifetimeComponent>();

  if(compJson->hasTimeLeft())
    self->timeLeft = compJson->getTimeLeft();
}

// HealthComponent
void HealthComponent::serialize(capnp::MessageBuilder& builder, Ref component) {
  auto healthBuilder = builder.initRoot<Schema::HealthComponent>();
  auto self = component.cast<HealthComponent>();
  healthBuilder.setHealth(self->health);
}

void HealthComponent::deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component) {
  auto healthReader = reader.getRoot<Schema::HealthComponent>();
  auto self = component.cast<HealthComponent>();
  self->health = healthReader.getHealth();
}

void HealthComponent::fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& jsonValue) {
  auto self = component.cast<HealthComponent>();
  auto compJson = jsonValue.as<JSchema::HealthComponent>();

  if(compJson->hasHealth())
    self->health = compJson->getHealth();
}

RAMPAGE_END