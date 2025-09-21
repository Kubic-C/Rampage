#pragma once

#include "../../common/common.hpp"

RAMPAGE_START

struct ContactDamageComponent {
  float damage = 10.0f;
};

struct BulletDamageComponent {
  float damage = 10.0f;
};

struct LifetimeComponent {
  float timeLeft = 1.0f;
};

struct HealthComponent {
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