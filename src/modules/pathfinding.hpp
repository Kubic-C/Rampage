#pragma once

#include "../components/worldMap.hpp"
#include "../components/health.hpp"
#include "../components/arrow.hpp"
#include "../components/tilemap.hpp"
#include "../components/collisionQueue.hpp"
#include "../components/body.hpp"

struct PathfindingModule : Module {
  static void registerComponents(EntityWorld& world) {
    world.set<ArrowComponent, PrimaryTargetTag, SeekPrimaryTargetTag>();
  }

  PathfindingModule(EntityWorld& world)
    : m_collisionQueue(world.create()) {
    m_collisionQueue.add<CollisionQueueComponent>();
  }

  ArrowComponent* getTopArrow(EntityWorld& world, RefT<TilemapComponent> tilemapLayers, const glm::i16vec2& pos) {
    for (int i = tilemapLayers->getTilemapCount(); i != 0; i--) {
      Tilemap& tilemap = tilemapLayers->getTilemap(i - 1);
      if (!tilemap.contains(pos))
        continue;

      Tile& tile = tilemap.find(pos);
      if(tile.entity == 0 || !world.get(tile.entity).has<ArrowComponent>())
        return nullptr;

      return &*world.get(tile.entity).get<ArrowComponent>();
    }

    return nullptr;
  }

  void run(EntityWorld& world, float deltaTime) override final {
    RefT<CollisionQueueComponent> queue = m_collisionQueue.get<CollisionQueueComponent>();
    for (CollisionQueueComponent::Collision& collision : queue->queue) {
      if (!world.exists(collision.primary))
        continue;
      Entity enemy = world.get(collision.primary);
      if (!world.exists(collision.secondary))
        continue;
      Entity other = world.get(collision.secondary);
      if (!other.has<HealthComponent>() || other.has<SeekPrimaryTargetTag>())
        continue;                                 

      RefT<HealthComponent> health = other.get<HealthComponent>();
      RefT<ContactDamageComponent> damage = enemy.get<ContactDamageComponent>();
      float speed = b2Length((b2Body_GetLinearVelocity(enemy.get<BodyComponent>()->id)));
      health->health -= (speed / 10.0f) * damage->damage;
    }
    queue->queue.clear();

    updateFlowField(world);

    Entity map = world.getWith(world.set<TransformComponent, TilemapComponent, WorldMapTag>()).next();
    RefT<TransformComponent> mapTransform = map.get<TransformComponent>();
    RefT<TilemapComponent> tilemapLayers = map.get<TilemapComponent>();
    EntityIterator it = world.getWith(world.set<TransformComponent, BodyComponent, SeekPrimaryTargetTag>());
    while (it.hasNext()) {
      Entity seeker = it.next();
      RefT<TransformComponent> seekerTransform = seeker.get<TransformComponent>();
      RefT<BodyComponent> seekerBody = seeker.get<BodyComponent>();

      Vec2 localMapPos = mapTransform->getLocalPoint(seekerTransform->pos);
      glm::i16vec2 localTilePos = Tilemap::getNearestTile(localMapPos);
      ArrowComponent* arrow = getTopArrow(world, tilemapLayers, localTilePos);
      if (!arrow)
        continue;

      b2Body_ApplyLinearImpulseToCenter(seekerBody->id, Vec2(arrow->dir * b2Body_GetMass(seekerBody->id)), true);
    }
  }

  void updateFlowField(EntityWorld& world) {
    Entity map = world.getWith(world.set<TransformComponent, TilemapComponent, WorldMapTag>()).next();
    RefT<TransformComponent> mapTransform = map.get<TransformComponent>();
    RefT<TilemapComponent> tilemapLayers = map.get<TilemapComponent>();
    Entity player = world.getWith(world.set<TransformComponent, PrimaryTargetTag>()).next();
    RefT<TransformComponent> playerTransform = player.get<TransformComponent>();

    Vec2 localMapPos = mapTransform->getLocalPoint(playerTransform->pos);
    glm::i16vec2 localTilePos = Tilemap::getNearestTile(localMapPos);
    if (tilemapLayers->getTopTilemapWith(localTilePos) == UINT32_MAX)
      return;

    ArrowComponent* startArrow = getTopArrow(world, tilemapLayers, localTilePos);
    if (!startArrow)
      return;
    startArrow->dir = glm::normalize(playerTransform->pos - mapTransform->getWorldPoint(Tilemap::getLocalTileCenter(localTilePos)));

    if (m_oldTarget == localTilePos)
      return;

    m_oldTarget = localTilePos;
    if (++m_currentGeneration == 0)
      m_currentGeneration = 1;

    std::queue<glm::i16vec2> openList;

    startArrow->cost = 0;
    startArrow->generation = m_currentGeneration;
    openList.emplace(localTilePos);

    while (!openList.empty()) {
      glm::i16vec2 current = openList.front();
      openList.pop();

      ArrowComponent* currentArrow = getTopArrow(world, tilemapLayers, current);

      for (int i = 0; i < directions.size(); ++i) {
        glm::i16vec2 neighbor = current + directions[i];

        ArrowComponent* neighborArrow = getTopArrow(world, tilemapLayers, neighbor);
        if (!neighborArrow)
          continue;

        float newCost = currentArrow->cost + costs[i] + neighborArrow->tileCost;
        if (neighborArrow->generation == m_currentGeneration && neighborArrow->cost <= newCost)
          continue;

        neighborArrow->cost = newCost;
        neighborArrow->dir = -normalizedDirs[i];
        neighborArrow->generation = m_currentGeneration;
        openList.emplace(neighbor);
      }
    }
  }

  Entity getContactDamageQueue() {
    return m_collisionQueue;
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
  Entity m_collisionQueue;
};