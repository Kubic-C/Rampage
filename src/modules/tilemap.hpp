#pragma once

#include "../components/tilemap.hpp"

class TilemapModule : public Module {
  public:
  static void registerComponents(EntityWorld& world) {
    world.component<TilemapComponent>();
    world.component<TileBoundComponent>();
    world.component<DestroyTileOnEntityRemovalTag>();

    world.observe(EntityWorld::EventType::Remove, world.component<DestroyTileOnEntityRemovalTag>(),
                  world.set<TileBoundComponent>(), [](Entity e) {
                    RefT<TileBoundComponent> tileBound = e.get<TileBoundComponent>();
                    RefT<TilemapComponent> tilemapLayers =
                        e.world().get(tileBound->parent).get<TilemapComponent>();
                    Tilemap& tilemap = tilemapLayers->getTilemap(tileBound->layer);

                    // Tiles may be destroyed already before we can delete them, hence the
                    // check
                    if (!tilemap.contains(tileBound->pos))
                      return;
                    if (tilemap.find(tileBound->pos).entity != e)
                      return;

                    tilemap.erase(tileBound->pos);
                  });

    world.observe(EntityWorld::EventType::Remove, world.component<TileBoundComponent>(), {}, [&](Entity e) {
      Map<EntityId, std::set<EntityId>>& ongoingCollisions =
          e.world().getModule<PhysicsModule>().getOngoingCollisions();

      auto it = ongoingCollisions.find(e);
      if (it != ongoingCollisions.end()) {
        for (EntityId other : it->second) {
          ongoingCollisions[other].erase(e);
          if (ongoingCollisions[other].empty())
            ongoingCollisions.erase(other);
        }
        ongoingCollisions.erase(it);
      }
    });

    world.observe(EntityWorld::EventType::Remove, world.component<TilemapComponent>(), {}, [&](Entity e) {
      RefT<TilemapComponent> tmLayers = e.get<TilemapComponent>();

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
    });
  }

  TilemapModule(EntityWorld& world) :
      m_calcTransforms(
          world.system(world.set<TransformComponent, TileBoundComponent>(), &updateTileBoundTransforms)) {}

  static void updateTileBoundTransforms(Entity e, float dt) {
    RefT<TransformComponent> transform = e.get<TransformComponent>();
    RefT<TileBoundComponent> tileBound = e.get<TileBoundComponent>();

    Entity parent = e.world().get(tileBound->parent);
    RefT<TransformComponent> parentTransform = parent.get<TransformComponent>();
    RefT<TilemapComponent> parentTilemapLayers = parent.get<TilemapComponent>();
    Tilemap& tilemap = parentTilemapLayers->getTilemap(tileBound->layer);
    Tile& tile = tilemap.find(tileBound->pos);

    *transform =
        Transform(parentTransform->getWorldPoint(tilemap.getLocalTileCenter(tileBound->pos, tile.size())),
                  transform->rot);
  }

  void run(EntityWorld& world, float deltaTime) override { m_calcTransforms.run(); }

  private:
  System m_calcTransforms;
};
