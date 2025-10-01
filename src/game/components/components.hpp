#pragma once

#include "arrow.hpp"
#include "body.hpp"
#include "collisionEvent.hpp"
#include "health.hpp"
#include "item.hpp"
#include "player.hpp"
#include "shapes.hpp"
#include "spawner.hpp"
#include "sprite.hpp"
#include "tilemap.hpp"
#include "turret.hpp"
#include "worldMap.hpp"

RAMPAGE_START

inline void registerGameComponents(EntityWorld& world) {
  world.component<ArrowComponent>();
  world.component<BodyComponent>();
  world.component<AddBodyComponent>();
  world.component<AddShapeComponent>();
  world.component<OnCollisionBeginEvent>();
  world.component<OnCollisionEndEvent>();
  world.component<HealthComponent>();
  world.component<LifetimeComponent>();
  world.component<BulletDamageComponent>();
  world.component<ContactDamageComponent>();
  world.component<ItemAttrStackCost>();
  world.component<ItemAttrUnique>();
  world.component<ItemAttrIcon>();
  world.component<ItemAttrTile>();
  world.component<TileItemComponent>();
  world.component<PlayerComponent>();
  world.component<CircleRenderComponent>();
  world.component<RectangleRenderComponent>();
  world.component<SpawnerComponent>();
  world.component<SpriteComponent>();
  world.component<SpriteIndependentTag>();
  world.component<TilePosComponent>();
  world.component<TurretComponent>();
  world.component<WorldMapTag>();
}

RAMPAGE_END
