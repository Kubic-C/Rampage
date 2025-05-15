#pragma once

#include "../utility/base.hpp"

struct ContactDamageComponent {
  float damage = 10.0f;
};

struct LifetimeComponent {
  float timeLeft = 1.0f;
};

struct HealthComponent {
  float health = 5.0f;
};

template <>
struct glz::meta<ContactDamageComponent>
{
  using T = ContactDamageComponent;
  static constexpr auto value = object("damage", &T::damage);
};

template <>
struct glz::meta<LifetimeComponent>
{
  using T = LifetimeComponent;
  static constexpr auto value = object("timeLeft", &T::timeLeft);
};

template <>
struct glz::meta<HealthComponent>
{
  using T = HealthComponent;
  static constexpr auto value = object("health", &T::health);
};