#pragma once

class EntityWorld;

class Module {
  public:
  virtual ~Module() {}

  virtual void run(EntityWorld& world, float deltaTime) {}

  private:
};
