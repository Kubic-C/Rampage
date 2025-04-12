#pragma once

#include "utility/ecs.hpp"

struct GrandUnitComponent {
  std::vector<EntityId> subUnits;
};

struct ArrowComponent {
  enum Direction {
    Up,
    Down,
    Left,
    Right
  };

  Direction dir;
};

struct PathfindingModule : Module {
  PathfindingModule(EntityWorld& world) {
    
  }

  void run(EntityWorld& world, float deltaTime) override final {

  }

private:
 
};