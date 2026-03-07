#include "pathfinding.hpp"

#include <queue>

#include "../module.hpp"

#include "../components/arrow.hpp"
#include "../components/body.hpp"
#include "../components/collisionEvent.hpp"
#include "../components/health.hpp"
#include "../components/tilemap.hpp"
#include "../components/worldMap.hpp"

RAMPAGE_START

struct PathfindingContext {
  u32 currentGeneration = 1;
  glm::ivec2 oldTarget = {0, 0};
};

constexpr std::array<glm::ivec2, 8> directions = {
    {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {-1, 1}, {-1, -1}, {1, -1}}};

const std::array<glm::vec2, 8> normalizedDirs = {
    glm::normalize(glm::vec2(directions[0])), glm::normalize(glm::vec2(directions[1])),
    glm::normalize(glm::vec2(directions[2])), glm::normalize(glm::vec2(directions[3])),
    glm::normalize(glm::vec2(directions[4])), glm::normalize(glm::vec2(directions[5])),
    glm::normalize(glm::vec2(directions[6])), glm::normalize(glm::vec2(directions[7]))};

const std::array<float, 8> costs = {1, 1, 1, 1, sqrtf(2), sqrtf(2), sqrtf(2), sqrtf(2)};

ArrowComponent* getTopArrow(IWorldPtr world, RefT<TilemapComponent> tilemap, const glm::ivec2& gridPos) {
  WorldLayer layer = tilemap->getLayerOfTopTile(gridPos);
  if (layer == WorldLayer::Invalid)
    return nullptr;

  EntityId tileId = tilemap->getTile(layer, gridPos);
  EntityPtr tileEntity = world->getEntity(tileId);
  
  if (tileEntity.has<ArrowComponent>())
    return &*tileEntity.get<ArrowComponent>();

  return nullptr;
}

bool isTileBlocked(IWorldPtr world, RefT<TilemapComponent> tilemap, const glm::ivec2& gridPos) {
  WorldLayer layer = tilemap->getLayerOfTopTile(gridPos);
  if (layer == WorldLayer::Invalid)
    return true;

  EntityId tileId = tilemap->getTile(layer, gridPos);
  EntityPtr tileEntity = world->getEntity(tileId);

  if (tileEntity.has<TileComponent>())
    return tileEntity.get<TileComponent>()->collidable;

  return true;
}

void updateFlowField(IWorldPtr world, EntityPtr map, PathfindingContext& context) {
  auto mapTransform = map.get<TransformComponent>();
  auto tilemap = map.get<TilemapComponent>();
  EntityPtr player = world->getFirstWith(world->set<TransformComponent, PrimaryTargetTag>());
  auto playerTransform = player.get<TransformComponent>();

  // Max tile-space Chebyshev distance for the flow field radius
  static constexpr int maxTileDistance = 10;

  Vec2 localMapPos = mapTransform->getLocalPoint(playerTransform->pos);
  glm::ivec2 localTilePos = getNearestTile(localMapPos);
  ArrowComponent* startArrow = getTopArrow(world, tilemap, localTilePos);
  if (!startArrow)
    return;
  startArrow->dir = glm::normalize(playerTransform->pos -
                                   mapTransform->getWorldPoint(getLocalTileCenter(localTilePos)));

  if (context.oldTarget == localTilePos)
    return;

  context.oldTarget = localTilePos;
  if (++context.currentGeneration == 0)
    context.currentGeneration = 1;

  struct Node {
    float cost;
    glm::ivec2 pos;
    bool operator>(const Node& other) const { return cost > other.cost; }
  };

  std::priority_queue<Node, std::vector<Node>, std::greater<Node>> openList;

  startArrow->cost = 0;
  startArrow->generation = context.currentGeneration;
  openList.push({0, localTilePos});

  while (!openList.empty()) {
    auto [currentCost, current] = openList.top();
    openList.pop();

    ArrowComponent* currentArrow = getTopArrow(world, tilemap, current);
    if (currentArrow->cost < currentCost)
      continue; // Stale entry — a shorter path was already found

    for (int i = 0; i < directions.size(); ++i) {
      glm::ivec2 neighbor = current + directions[i];

      // Prevent diagonal corner-cutting through walls
      if (i >= 4) {
        if (isTileBlocked(world, tilemap, current + glm::ivec2(directions[i].x, 0)) ||
            isTileBlocked(world, tilemap, current + glm::ivec2(0, directions[i].y)))
          continue;
      }

      ArrowComponent* neighborArrow = getTopArrow(world, tilemap, neighbor);
      if (!neighborArrow)
        continue;

      float newCost = currentCost + costs[i] + neighborArrow->tileCost;
      if (neighborArrow->generation == context.currentGeneration && neighborArrow->cost <= newCost)
        continue;

      glm::ivec2 diff = glm::abs(neighbor - localTilePos);
      int tileDistance = glm::max(diff.x, diff.y);
      if (tileDistance > maxTileDistance && neighborArrow->generation != 0)
        continue;

      neighborArrow->cost = newCost;
      neighborArrow->dir = -normalizedDirs[i];
      neighborArrow->generation = context.currentGeneration;
      openList.push({newCost, neighbor});
    }
  }
}

int updatePathfinding(IWorldPtr world, float deltaTime) {
  auto& context = world->getContext<PathfindingContext>();

  EntityPtr player = world->getFirstWith(world->set<TransformComponent, PrimaryTargetTag>());
  if (!player.isNull()) {
    auto playerTransform = player.get<TransformComponent>();
    EntityPtr onTile = getTileAtPos(world, playerTransform->pos);
    if(onTile.isNull())
      return 0;
    auto tileComp = onTile.get<TileComponent>();
    EntityPtr tm = world->getEntity(tileComp->parent);
    updateFlowField(world, tm, context);
  }

  IEntityIteratorPtr it = world->getWith(world->set<TransformComponent, BodyComponent, SeekPrimaryTargetTag>());
  while (it->hasNext()) {
    EntityPtr seeker = it->next();
    RefT<TransformComponent> seekerTransform = seeker.get<TransformComponent>();
    RefT<BodyComponent> seekerBody = seeker.get<BodyComponent>();

    EntityPtr onTile = getTileAtPos(world, seekerTransform->pos);
    if(onTile.isNull())
      continue;
    auto tileComp = onTile.get<TileComponent>();
    EntityPtr tilemapEntity = world->getEntity(tileComp->parent);
    auto mapTransform = tilemapEntity.get<TransformComponent>();
    auto tilemap = tilemapEntity.get<TilemapComponent>();

    Vec2 localMapPos = mapTransform->getLocalPoint(seekerTransform->pos);
    glm::ivec2 localTilePos = getNearestTile(localMapPos);
    ArrowComponent* arrow = getTopArrow(world, tilemap, localTilePos);
    if (!arrow)
      continue;

    Vec2 actualDir = mapTransform->rot.rotate(arrow->dir);
    float massSpeed = glm::min(5.0f / b2Body_GetMass(seekerBody->id), 0.75f);
    b2Body_ApplyLinearImpulseToCenter(seekerBody->id, Vec2(actualDir * massSpeed), true);
    seekerTransform->rot = atan2(actualDir.y, actualDir.x);
  }

  return 0;
}

void observeContactDamageCollision(EntityPtr enemy) {
  auto world = enemy.world();
  auto collisionData = enemy.get<LastCollisionData>();

  EntityPtr other = world->getEntity(collisionData->other);
  if (!other.has<HealthComponent>() || other.has<ContactDamageComponent>())
    return;

  auto damage = enemy.get<ContactDamageComponent>();
  auto health = other.get<HealthComponent>();
  health->health -= damage->damage;
}

void loadPathfindingSystems(IHost& host) {
  Pipeline& pipeline = host.getPipeline();
  IWorldPtr world = host.getWorld();

  world->addContext<PathfindingContext>();
  auto& context = world->getContext<PathfindingContext>();

  world->observe<OnCollisionBeginEvent>(world->component<LastCollisionData>(),
                world->set<ContactDamageComponent>(), observeContactDamageCollision);

  pipeline.getGroup<GameGroup>().attachToStage<GameGroup::TickStage>(updatePathfinding);
}

RAMPAGE_END
