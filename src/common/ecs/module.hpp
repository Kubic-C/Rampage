#pragma once

#include "../commondef.hpp"

RAMPAGE_START

class EntityWorld;

class Module {
  public:
  virtual ~Module() {}

  virtual void run(EntityWorld& world, float deltaTime) {}

  private:
};

RAMPAGE_END