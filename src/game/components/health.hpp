#pragma once

#include "../../common/common.hpp"

RAMPAGE_START

struct ContactDamageComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component) {
    auto contactDmgBuilder = builder.initRoot<Schema::ContactDamageComponent>();
    auto self = component.cast<ContactDamageComponent>();
    contactDmgBuilder.setDamage(self->damage);
  }

  static void deserialize(capnp::MessageReader& reader, Ref component) {
    auto contactDmgReader = reader.getRoot<Schema::ContactDamageComponent>();
    auto self = component.cast<ContactDamageComponent>();
    self->damage = contactDmgReader.getDamage();
  }

  float damage = 10.0f;
};

struct BulletDamageComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component) {
    auto bulletDmgBuilder = builder.initRoot<Schema::BulletDamageComponent>();
    auto self = component.cast<BulletDamageComponent>();
    bulletDmgBuilder.setDamage(self->damage);
  }

  static void deserialize(capnp::MessageReader& reader, Ref component) {
    auto bulletDmgReader = reader.getRoot<Schema::BulletDamageComponent>();
    auto self = component.cast<BulletDamageComponent>();
    self->damage = bulletDmgReader.getDamage();
  }

  float damage = 10.0f;
};

struct LifetimeComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component) {
    auto lifetimeBuilder = builder.initRoot<Schema::LifetimeComponent>();
    auto self = component.cast<LifetimeComponent>();
    lifetimeBuilder.setTimeLeft(self->timeLeft);
  }

  static void deserialize(capnp::MessageReader& reader, Ref component) {
    auto lifetimeReader = reader.getRoot<Schema::LifetimeComponent>();
    auto self = component.cast<LifetimeComponent>();
    self->timeLeft = lifetimeReader.getTimeLeft();
  }

  float timeLeft = 1.0f;
};

struct HealthComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component) {
    auto healthBuilder = builder.initRoot<Schema::HealthComponent>();
    auto self = component.cast<HealthComponent>();
    healthBuilder.setHealth(self->health);
  }

  static void deserialize(capnp::MessageReader& reader, Ref component) {
    auto healthReader = reader.getRoot<Schema::HealthComponent>();
    auto self = component.cast<HealthComponent>();
    self->health = healthReader.getHealth();
  }

  float health = 5.0f;
};

RAMPAGE_END

template <>
struct glz::meta<rmp::ContactDamageComponent> {
  using T = rmp::ContactDamageComponent;
  static constexpr auto value = object("damage", &T::damage);
};

template <>
struct glz::meta<rmp::LifetimeComponent> {
  using T = rmp::LifetimeComponent;
  static constexpr auto value = object("timeLeft", &T::timeLeft);
};

template <>
struct glz::meta<rmp::HealthComponent> {
  using T = rmp::HealthComponent;
  static constexpr auto value = object("health", &T::health);
};