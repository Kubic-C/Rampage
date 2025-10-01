#include "tilemap.hpp"
#include "../components/tilemap.hpp"
#include "../module.hpp"

RAMPAGE_START

void removeTileIfEntityDestroyed(Entity e) {
  auto tileBound = e.get<TileBoundComponent>();
  auto tilemapLayers = e.world().get(tileBound->parent).get<TilemapComponent>();
  Tilemap& tilemap = tilemapLayers->getTilemap(tileBound->layer);

  // Tiles may be destroyed already before we can delete them, hence the
  // check
  if (!tilemap.contains(tileBound->pos))
    return;
  if (tilemap.find(tileBound->pos).entity != e)
    return;

  tilemap.erase(tileBound->pos);
}

void destroyTileBoundEntities(Entity e) {
  auto tmLayers = e.get<TilemapComponent>();

  std::vector<glm::i16vec2> toDestroy;
  for (int i = 0; i < tmLayers->getTilemapCount(); i++) {
    Tilemap& tilemap = tmLayers->getTilemap(i);

    toDestroy.clear();
    for (glm::i16vec2 pos : tilemap) {
      toDestroy.push_back(pos);
    }

    for (glm::i16vec2 pos : toDestroy) {
      EntityId id = tilemap.erase(pos);
      if (id == 0)
        continue;
      e.world().destroy(id);
    }
  }
}

void updateTileBoundTransforms(Entity e) {
  auto transform = e.get<TransformComponent>();
  auto tileBound = e.get<TileBoundComponent>();

  Entity parent = e.world().get(tileBound->parent);
  auto parentTransform = parent.get<TransformComponent>();
  auto parentTilemapLayers = parent.get<TilemapComponent>();
  Tilemap& tilemap = parentTilemapLayers->getTilemap(tileBound->layer);
  Tile& tile = tilemap.find(tileBound->pos);

  *transform =
      Transform(parentTransform->getWorldPoint(tilemap.getLocalTileCenter(tileBound->pos, tile.size())),
                transform->rot);
}

int updateTileBoundTransformsForAll(EntityWorld& world, float dt) {
  auto it = world.getWith(world.set<TransformComponent, TileBoundComponent>());
  while (it.hasNext())
    updateTileBoundTransforms(it.next());

  return 0;
}

void loadTilemapSystems(IHost& host) {
  Pipeline& pipeline = host.getPipeline();
  EntityWorld& world = host.getWorld();

  world.observe<ComponentRemovedEvent>(world.component<DestroyTileOnEntityRemovalTag>(),
                world.set<TileBoundComponent>(), removeTileIfEntityDestroyed);
  world.observe<ComponentRemovedEvent>(world.component<TilemapComponent>(), {},
                destroyTileBoundEntities);

  pipeline.getGroup<GameGroup>().attachToStage<GameGroup::TickStage>(updateTileBoundTransformsForAll);
}

RAMPAGE_END
