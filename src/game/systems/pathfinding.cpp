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

constexpr std::array<glm::ivec2, 8> directions = {
    {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {-1, 1}, {-1, -1}, {1, -1}}};

const std::array<glm::vec2, 8> normalizedDirs = {
    glm::normalize(glm::vec2(directions[0])), glm::normalize(glm::vec2(directions[1])),
    glm::normalize(glm::vec2(directions[2])), glm::normalize(glm::vec2(directions[3])),
    glm::normalize(glm::vec2(directions[4])), glm::normalize(glm::vec2(directions[5])),
    glm::normalize(glm::vec2(directions[6])), glm::normalize(glm::vec2(directions[7]))};

const std::array<float, 8> costs = {1, 1, 1, 1, sqrtf(2), sqrtf(2), sqrtf(2), sqrtf(2)};

bool hasTopArrow(IWorldPtr world, RefT<TilemapComponent> tilemap, const glm::ivec2& gridPos) {
  WorldLayer layer = tilemap->getLayerOfTopTile(gridPos);
  if (layer == WorldLayer::Invalid)
    return false;
  EntityId tileId = tilemap->getTile(layer, gridPos).tileId;
  EntityPtr tileEntity = world->getEntity(tileId);
  return tileEntity.has<ArrowComponent>();
}

RefT<ArrowComponent> getTopArrow(IWorldPtr world, RefT<TilemapComponent> tilemap, const glm::ivec2& gridPos) {
  WorldLayer layer = tilemap->getLayerOfTopTile(gridPos);

  EntityId tileId = tilemap->getTile(layer, gridPos).tileId;
  EntityPtr tileEntity = world->getEntity(tileId);
  
  return tileEntity.get<ArrowComponent>();
}

bool isTileBlocked(IWorldPtr world, RefT<TilemapComponent> tilemap, const glm::ivec2& gridPos) {
  WorldLayer layer = tilemap->getLayerOfTopTile(gridPos);
  if (layer == WorldLayer::Invalid)
    return true;

  EntityId tileId = tilemap->getTile(layer, gridPos).tileId;
  EntityPtr tileEntity = world->getEntity(tileId);

  if (tileEntity.has<TileComponent>())
    return tileEntity.get<TileComponent>()->collidable;

  return true;
}

void updateFlowField(IWorldPtr world, EntityPtr map) {
  auto mapTransform = map.get<TransformComponent>();
  auto tilemap = map.get<TilemapComponent>();
  auto pathfinding = map.get<VectorTilemapPathfinding>();
  EntityPtr player = world->getFirstWith(world->set<TransformComponent, PrimaryTargetTag>());
  auto playerTransform = player.get<TransformComponent>();

  // Max tile-space Chebyshev distance for the flow field radius
  static constexpr int maxTileDistance = 50;

  Vec2 localMapPos = mapTransform->getLocalPoint(playerTransform->pos);
  glm::ivec2 localTilePos = getNearestTile(localMapPos);
  if (!hasTopArrow(world, tilemap, localTilePos))
    return;
  auto startArrow = getTopArrow(world, tilemap, localTilePos);
  pathfinding->nodes[localTilePos].dir =
      glm::normalize(playerTransform->pos -
                                   mapTransform->getWorldPoint(getLocalTileCenter(localTilePos)));

  if (pathfinding->oldTarget == localTilePos)
    return;

  pathfinding->oldTarget = localTilePos;
  ++pathfinding->curGen;

  struct Node {
    float cost;
    glm::ivec2 pos;
    bool operator>(const Node& other) const { return cost > other.cost; }
  };

  std::priority_queue<Node, std::vector<Node>, std::greater<Node>> openList;

  pathfinding->nodes[localTilePos].cost = 0;
  pathfinding->nodes[localTilePos].gen = pathfinding->curGen;
  openList.push({0, localTilePos});

  while (!openList.empty()) {
    auto [currentCost, current] = openList.top();
    openList.pop();

    auto curArrow = getTopArrow(world, tilemap, current);
    if (pathfinding->nodes[current].cost < currentCost)
      continue; // Stale entry — a shorter path was already found

    for (int i = 0; i < directions.size(); ++i) {
      glm::ivec2 neighbor = current + directions[i];

      // Prevent diagonal corner-cutting through walls
      if (i >= 4) {
        if (isTileBlocked(world, tilemap, current + glm::ivec2(directions[i].x, 0)) ||
            isTileBlocked(world, tilemap, current + glm::ivec2(0, directions[i].y)))
          continue;
      }

      if (!hasTopArrow(world, tilemap, neighbor))
        continue;

      auto neighborArrow = getTopArrow(world, tilemap, neighbor);
      float newCost = currentCost + costs[i] + neighborArrow->tileCost;
      if (pathfinding->nodes[neighbor].gen == pathfinding->curGen && pathfinding->nodes[neighbor].cost <= newCost)
        continue;

      glm::ivec2 diff = glm::abs(neighbor - localTilePos);
      int tileDistance = glm::max(diff.x, diff.y);
      if (tileDistance > maxTileDistance && pathfinding->nodes[neighbor].gen != 0)
        continue;

      pathfinding->nodes[neighbor].cost = newCost;
      pathfinding->nodes[neighbor].dir = -normalizedDirs[i];
      pathfinding->nodes[neighbor].gen = pathfinding->curGen;
      openList.push({newCost, neighbor});
    }
  }
}

int updatePathfinding(IWorldPtr world, float deltaTime) {
  EntityPtr player = world->getFirstWith(world->set<TransformComponent, PrimaryTargetTag>());
  if (player.isNull())
    return 0;

  auto playerTransform = player.get<TransformComponent>();
  TileRef onTile = getTileAtPos(world, playerTransform->pos);
  if (onTile.entity == NullEntityId)
    return 0;
  EntityPtr tilemapEntity = world->getEntity(onTile.tilemap);
  if (!tilemapEntity.has<VectorTilemapPathfinding>())
    return 0;
  updateFlowField(world, tilemapEntity);

  return 0;
}

int updatePathfindingMovement(IWorldPtr world, float deltaTime) {
  IEntityIteratorPtr it = world->getWith(world->set<TransformComponent, BodyComponent, SeekPrimaryTargetTag>());
  while (it->hasNext()) {
    EntityPtr seeker = it->next();
    RefT<TransformComponent> seekerTransform = seeker.get<TransformComponent>();
    RefT<BodyComponent> seekerBody = seeker.get<BodyComponent>();

    TileRef onTile = getTileAtPos(world, seekerTransform->pos);
    if(onTile.entity == NullEntityId)
      continue;
    EntityPtr tilemapEntity = world->getEntity(onTile.tilemap);
    auto mapTransform = tilemapEntity.get<TransformComponent>();
    if (!tilemapEntity.has<VectorTilemapPathfinding>())
      continue;
    auto pathfinding = tilemapEntity.get<VectorTilemapPathfinding>();

    Vec2 localMapPos = mapTransform->getLocalPoint(seekerTransform->pos);
    glm::ivec2 localTilePos = getNearestTile(localMapPos);
    if (!pathfinding->nodes.contains(onTile.worldPos))
      continue;

    Vec2 actualDir = mapTransform->rot.rotate(pathfinding->nodes[onTile.worldPos].dir);
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

  world->observe<OnCollisionBeginEvent>(world->component<LastCollisionData>(),
                world->set<ContactDamageComponent>(), observeContactDamageCollision);

  pipeline.getGroup<GamePerSecondGroup>().attachToStage<GamePerSecondGroup::TickStage>(updatePathfinding);
  pipeline.getGroup<GameGroup>().attachToStage<GameGroup::TickStage>(updatePathfindingMovement);
}

RAMPAGE_END
