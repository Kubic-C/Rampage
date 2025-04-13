#pragma once

#include "utility/ecs.hpp"
#include "transform.hpp"
#include "worldMap.hpp"

struct PrimaryTargetTag {};

struct GrandUnitComponent {
  std::vector<EntityId> subUnits;
};

struct ArrowComponent {
  // Normalized vector
  // points up, down, left, or right
  Vec2 dir = { 1.0f, 0.0f };
  size_t cost = SIZE_MAX;
};

struct SeekPrimaryTargetTag {};

struct PathfindingModule : Module {
  PathfindingModule(EntityWorld& world) {
    world.set<GrandUnitComponent, ArrowComponent, PrimaryTargetTag, SeekPrimaryTargetTag>();
  }

  void run(EntityWorld& world, float deltaTime) override final {
    updateFlowField(world);

    Entity map = world.getWith(world.set<PosComponent, RotComponent, TilemapComponent, WorldMapTag>()).next();
    PosComponent& mapPos = map.get<PosComponent>();
    RotComponent& mapRot = map.get<RotComponent>();
    TilemapComponent& tilemap = map.get<TilemapComponent>();
    EntityIterator it = world.getWith(world.set<PosComponent, BodyComponent, SeekPrimaryTargetTag>());
    while (it.hasNext()) {
      Entity seeker = it.next();
      PosComponent& seekerPos = seeker.get<PosComponent>();
      BodyComponent& seekerBody = seeker.get<BodyComponent>();

      Vec2 localMapPos = Transform(mapPos, mapRot).getLocalPoint(seekerPos);
      glm::i16vec2 localTilePos = tilemap.getNearestTile(localMapPos);
      if (!tilemap.contains(localTilePos))
        continue;
      ArrowComponent& arrow = world.get(tilemap.find(localTilePos).entity).get<ArrowComponent>();

      b2Body_ApplyLinearImpulseToCenter(seekerBody.id, Vec2(arrow.dir * b2Body_GetMass(seekerBody.id)), true);
    }
  }

  void updateFlowField(EntityWorld& world) {
    Entity map = world.getWith(world.set<PosComponent, RotComponent, TilemapComponent, WorldMapTag>()).next();
    PosComponent& mapPos = map.get<PosComponent>();
    RotComponent& mapRot = map.get<RotComponent>();
    TilemapComponent& tilemap = map.get<TilemapComponent>();
    Entity player = world.getWith(world.set<PosComponent, RotComponent, PrimaryTargetTag>()).next();
    PosComponent& playerPos = player.get<PosComponent>();
    RotComponent& playerRot = player.get<RotComponent>();

    Vec2 localMapPos = Transform(mapPos, mapRot).getLocalPoint(playerPos);
    glm::i16vec2 localTilePos = tilemap.getNearestTile(localMapPos);
    if(m_oldTarget == localTilePos)
      return;
    m_oldTarget = localTilePos;

    for(glm::i16vec2 tilePos : tilemap) {
      Entity e = world.get(tilemap.find(tilePos).entity);
      if (!e.has<ArrowComponent>())
        continue;
      ArrowComponent& tile = e.get<ArrowComponent>();
      tile.cost = SIZE_MAX;
    }

    std::queue<glm::i16vec2> openList;
    openList.push(localTilePos); // Start from the target
    world.get(tilemap.find(localTilePos).entity).get<ArrowComponent>().cost = 0;

    constexpr std::array<glm::i16vec2, 8> directions = {
      { {1, 0}, {-1, 0}, { 0,  1}, {0, -1},
        {1, 1}, {-1, 1}, {-1, -1}, {1, -1} }
    };

    const std::array<glm::vec2, 8> normalizedDirs = {
      glm::normalize(glm::vec2(directions[0])),
      glm::normalize(glm::vec2(directions[1])),
      glm::normalize(glm::vec2(directions[2])),
      glm::normalize(glm::vec2(directions[3])),
      glm::normalize(glm::vec2(directions[4])),
      glm::normalize(glm::vec2(directions[5])),
      glm::normalize(glm::vec2(directions[6])),
      glm::normalize(glm::vec2(directions[7]))
    };

    while(!openList.empty()) {
      glm::i16vec2 current = openList.front();
      openList.pop();

      Entity currentEntity = world.get(tilemap.find(current).entity);
      ArrowComponent& currentArrow = currentEntity.get<ArrowComponent>();
      size_t currentCost = currentArrow.cost;

      for(int i = 0; i < directions.size(); i++) {
        glm::i16vec2 neighbor = current + directions[i];

        // Skip invalid or non-existent tiles
        if(!tilemap.contains(neighbor))
          continue;

        auto neighborTile = tilemap.find(neighbor);
        Entity neighborEntity = world.get(neighborTile.entity);
        if(!neighborEntity.has<ArrowComponent>() || 
           tilemap.find(neighbor).flags & TileFlags::IS_COLLIDABLE)
          continue;

        ArrowComponent& neighborArrow = neighborEntity.get<ArrowComponent>();
        if(neighborArrow.cost <= currentCost + 1)
          continue;

        neighborArrow.cost = currentCost + 1;
        neighborArrow.dir = -normalizedDirs[i];
        openList.push(neighbor);
      }
    }
  }

private:
  glm::i16vec2 m_oldTarget = { 0, 0 };
};