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
  float cost = std::numeric_limits<float>::max();
  u32 generation = 0;
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

  using QueueItem = std::pair<float, glm::i16vec2>;

  struct CompareQueueItem {
    bool operator()(const QueueItem& a, const QueueItem& b) const {
      return a.first > b.first; // min-heap based on cost
    }
  };

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

    Tile& startTile = tilemap.find(localTilePos);
    Entity startEntity = world.get(startTile.entity);
    ArrowComponent& startArrow = startEntity.get<ArrowComponent>();
    startArrow.dir = glm::normalize(playerPos - Transform(mapPos, mapRot).getWorldPoint(tilemap.getLocalTileCenter(localTilePos)));

    if (m_oldTarget == localTilePos)
      return;

    m_oldTarget = localTilePos;
    if (++m_currentGeneration == 0)
      m_currentGeneration = 1;

    using QueueItem = std::pair<float, glm::i16vec2>;
    std::priority_queue<QueueItem, std::vector<QueueItem>, CompareQueueItem> openList;

    startArrow.cost = 0;
    startArrow.generation = m_currentGeneration;
    openList.emplace(0.0f, localTilePos);

    while (!openList.empty()) {
      auto [currentCost, current] = openList.top();
      openList.pop();

      Tile& currentTile = tilemap.find(current);
      Entity currentEntity = world.get(currentTile.entity);
      ArrowComponent& currentArrow = currentEntity.get<ArrowComponent>();

      for (int i = 0; i < directions.size(); ++i) {
        glm::i16vec2 neighbor = current + directions[i];

        if (!tilemap.contains(neighbor))
          continue;

        Tile& neighborTile = tilemap.find(neighbor);
        if (neighborTile.flags & TileFlags::IS_COLLIDABLE)
          continue;

        Entity neighborEntity = world.get(neighborTile.entity);
        if (!neighborEntity.has<ArrowComponent>())
          continue;

        ArrowComponent& neighborArrow = neighborEntity.get<ArrowComponent>();

        float newCost = currentCost + costs[i];
        if (neighborArrow.generation == m_currentGeneration && neighborArrow.cost <= newCost)
          continue;

        neighborArrow.cost = newCost;
        neighborArrow.dir = -normalizedDirs[i];
        neighborArrow.generation = m_currentGeneration;

        openList.emplace(newCost, neighbor);
      }
    }
  }

private:
  const std::array<glm::i16vec2, 8> directions = {
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

  const std::array<float, 8> costs = {
    1,
    1,
    1,
    1,
    sqrtf(2),
    sqrtf(2),
    sqrtf(2),
    sqrtf(2)
  };

private:
  u32 m_currentGeneration = 1;
  glm::i16vec2 m_oldTarget = { 0, 0 };
};