#pragma once

#include "arrow.hpp"
#include "body.hpp"
#include "collisionEvent.hpp"
#include "health.hpp"
#include "player.hpp"
#include "shapes.hpp"
#include "spawner.hpp"
#include "sprite.hpp"
#include "tilemap.hpp"
#include "chunkedTilemap.hpp"
#include "turret.hpp"
#include "worldMap.hpp"
#include "inventory.hpp"    
#include "ownedBy.hpp"

RAMPAGE_START

inline void registerGameComponents(IWorldPtr world) {
  world->component<BodyComponent>(false); // <- loaded first.
  world->component<ArrowComponent>(false);
  world->component<OnCollisionBeginEvent>(false);
  world->component<OnCollisionEndEvent>(false);
  world->component<HealthComponent>(false);
  world->component<LifetimeComponent>(false);
  world->component<BulletDamageComponent>(false);
  world->component<ContactDamageComponent>(false);
  world->component<PlayerComponent>(false);
  world->component<CircleRenderComponent>(false);
  world->component<RectangleRenderComponent>(false);
  world->component<SpawnerComponent>(false);
  world->component<SpriteComponent>(false);
  world->component<MultiTileComponent>(false);
  world->component<TileComponent>(false);
  world->component<TilemapComponent>(false);
  world->component<ChunkedTilemapComponent>(false);
  world->component<TurretComponent>(false);
  world->component<WorldMapTag>(false);
  world->component<SeekPrimaryTargetTag>(false);
  world->component<PrimaryTargetTag>(false);
  world->component<LastCollisionData>(false);
  world->component<InventoryComponent>(false);
  world->component<ItemComponent>(false);
  world->component<InventoryViewComponent>(false);
  world->component<ItemStackComponent>(false);
  world->component<ItemPlaceableComponent>(false);
  world->component<ItemPlacedTag>(false);
  world->component<ItemDroppedTag>(false);
  world->component<OwnedByComponent>(false);
  world->component<ConveyorPartComponent>(false);
  world->component<PortComponent>(false);
  world->component<ConveyorComponent>(false);
  world->component<ChunkLoaderTag>(false);
}

RAMPAGE_END
