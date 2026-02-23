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

inline void registerGameComponents(IWorldPtr world) {
  world->component<BodyComponent>(false); // <- loaded first.
  world->component<ArrowComponent>(false);
  world->component<AddBodyComponent>(false);
  world->component<AddShapeComponent>(false);
  world->component<OnCollisionBeginEvent>(false);
  world->component<OnCollisionEndEvent>(false);
  world->component<HealthComponent>(false);
  world->component<LifetimeComponent>(false);
  world->component<BulletDamageComponent>(false);
  world->component<ContactDamageComponent>(false);
  world->component<ItemAttrStackCost>(false);
  world->component<ItemAttrUnique>(false);
  world->component<ItemAttrIcon>(false);
  world->component<ItemAttrTile>(false);
  world->component<TileItemComponent>(false);
  world->component<PlayerComponent>(false);
  world->component<CircleRenderComponent>(false);
  world->component<RectangleRenderComponent>(false);
  world->component<SpawnerComponent>(false);
  world->component<SpriteComponent>(false);
  world->component<SpriteIndependentTag>(false);
  world->component<TilemapComponent>(false);
  world->component<DestroyTileOnEntityRemovalTag>(false);
  world->component<TileBoundComponent>(false);
  world->component<TilePosComponent>(false);
  world->component<TurretComponent>(false);
  world->component<WorldMapTag>(false);
  world->component<SeekPrimaryTargetTag>(false);
  world->component<PrimaryTargetTag>(false);
  world->component<LastCollisionData>(false);
  world->component<InventoryComponent>(false);
}

RAMPAGE_END
