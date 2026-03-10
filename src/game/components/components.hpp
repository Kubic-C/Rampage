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
  world->registerComponent<BodyComponent>(); // <- loaded first.
  world->registerComponent<ArrowComponent>();
  world->registerComponent<VectorTilemapPathfinding>();
  world->registerComponent<OnCollisionBeginEvent>();
  world->registerComponent<OnCollisionEndEvent>();
  world->registerComponent<HealthComponent>();
  world->registerComponent<LifetimeComponent>();
  world->registerComponent<BulletDamageComponent>();
  world->registerComponent<ContactDamageComponent>();
  world->registerComponent<PlayerComponent>();
  world->registerComponent<CircleRenderComponent>();
  world->registerComponent<RectangleRenderComponent>();
  world->registerComponent<SpawnerComponent>();
  world->registerComponent<SpriteComponent>();
  world->registerComponent<MultiTileComponent>();
  world->registerComponent<TileComponent>();
  world->registerComponent<TilemapComponent>();
  world->registerComponent<ChunkedTilemapComponent>();
  world->registerComponent<TurretComponent>();
  world->registerComponent<WorldMapTag>();
  world->registerComponent<SeekPrimaryTargetTag>();
  world->registerComponent<PrimaryTargetTag>();
  world->registerComponent<LastCollisionData>();
  world->registerComponent<InventoryComponent>();
  world->registerComponent<ItemComponent>();
  world->registerComponent<InventoryViewComponent>();
  world->registerComponent<ItemStackComponent>();
  world->registerComponent<ItemPlaceableComponent>();
  world->registerComponent<ItemPlacedTag>();
  world->registerComponent<ItemDroppedTag>();
  world->registerComponent<OwnedByComponent>();
  world->registerComponent<ConveyorPartComponent>();
  world->registerComponent<PortComponent>();
  world->registerComponent<ConveyorComponent>();
  world->registerComponent<ChunkLoaderTag>();
  world->registerComponent<UniqueTileComponent>();
}

RAMPAGE_END
