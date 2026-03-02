#include "pathfinding.hpp"

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
  glm::ivec3 oldTarget = {0, 0, 0};
};

constexpr std::array<glm::ivec3, 8> directions = {
    {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {1, 1, 0}, {-1, 1, 0}, {-1, -1, 0}, {1, -1, 0}}};

const std::array<glm::vec2, 8> normalizedDirs = {
    glm::normalize(glm::vec2(directions[0])), glm::normalize(glm::vec2(directions[1])),
    glm::normalize(glm::vec2(directions[2])), glm::normalize(glm::vec2(directions[3])),
    glm::normalize(glm::vec2(directions[4])), glm::normalize(glm::vec2(directions[5])),
    glm::normalize(glm::vec2(directions[6])), glm::normalize(glm::vec2(directions[7]))};

const std::array<float, 8> costs = {1, 1, 1, 1, sqrtf(2), sqrtf(2), sqrtf(2), sqrtf(2)};

ArrowComponent* getTopArrow(IWorldPtr world, RefT<TilemapComponent> tilemap, const glm::ivec3& gridPos) {
  // Check from highest Z layer downward to find top arrow
  for (int z = TilemapComponent::maxTopLayer; z >= TilemapComponent::minTopLayer; z--) {
    glm::ivec3 pos = {gridPos.x, gridPos.y, z};
    
    if (!tilemap->tiles.contains(pos))
      continue;

    EntityId tileId = tilemap->tiles.at(pos);
    EntityPtr tileEntity = world->getEntity(tileId);
    
    if (tileEntity.has<ArrowComponent>())
      return &*tileEntity.get<ArrowComponent>();
  }

  return nullptr;
}

void updateFlowField(IWorldPtr world, EntityPtr map, PathfindingContext& context) {
  auto mapTransform = map.get<TransformComponent>();
  auto tilemap = map.get<TilemapComponent>();
  EntityPtr player = world->getFirstWith(world->set<TransformComponent, PrimaryTargetTag>());
  if (player.isNull())
    return;

  auto playerTransform = player.get<TransformComponent>();

  Vec2 localMapPos = mapTransform->getLocalPoint(playerTransform->pos);
  glm::ivec3 localTilePos = TilemapComponent::getNearestTile(localMapPos);
  ArrowComponent* startArrow = getTopArrow(world, tilemap, localTilePos);
  if (!startArrow)
    return;
  startArrow->dir = glm::normalize(playerTransform->pos -
                                   mapTransform->getWorldPoint(TilemapComponent::getLocalTileCenter(localTilePos)));

  if (context.oldTarget == localTilePos)
    return;

  context.oldTarget = localTilePos;
  if (++context.currentGeneration == 0)
    context.currentGeneration = 1;

  std::queue<glm::ivec3> openList;

  startArrow->cost = 0;
  startArrow->generation = context.currentGeneration;
  openList.emplace(localTilePos);

  while (!openList.empty()) {
    glm::ivec3 current = openList.front();
    openList.pop();

    ArrowComponent* currentArrow = getTopArrow(world, tilemap, current);

    for (int i = 0; i < directions.size(); ++i) {
      glm::ivec3 neighbor = current + directions[i];

      ArrowComponent* neighborArrow = getTopArrow(world, tilemap, neighbor);
      if (!neighborArrow)
        continue;

      float newCost = currentArrow->cost + costs[i] + neighborArrow->tileCost;
      if (neighborArrow->generation == context.currentGeneration && neighborArrow->cost <= newCost)
        continue;

      neighborArrow->cost = newCost;
      neighborArrow->dir = -normalizedDirs[i];
      neighborArrow->generation = context.currentGeneration;
      openList.emplace(neighbor);
    }
  }
}

int updatePathfinding(IWorldPtr world, float deltaTime) {
  auto& context = world->getContext<PathfindingContext>();

  EntityPtr map = world->getFirstWith(world->set<TransformComponent, TilemapComponent, WorldMapTag>());
  if (map.isNull())
    return 0;

  updateFlowField(world, map, context);

  RefT<TransformComponent> mapTransform = map.get<TransformComponent>();
  RefT<TilemapComponent> tilemap = map.get<TilemapComponent>();
  IEntityIteratorPtr it = world->getWith(world->set<TransformComponent, BodyComponent, SeekPrimaryTargetTag>());
  while (it->hasNext()) {
    EntityPtr seeker = it->next();
    RefT<TransformComponent> seekerTransform = seeker.get<TransformComponent>();
    RefT<BodyComponent> seekerBody = seeker.get<BodyComponent>();

    Vec2 localMapPos = mapTransform->getLocalPoint(seekerTransform->pos);
    glm::ivec3 localTilePos = TilemapComponent::getNearestTile(localMapPos);
    ArrowComponent* arrow = getTopArrow(world, tilemap, localTilePos);
    if (!arrow)
      continue;

    float massSpeed = glm::min(5.0f / b2Body_GetMass(seekerBody->id), 0.75f);
    b2Body_ApplyLinearImpulseToCenter(seekerBody->id, Vec2(arrow->dir * massSpeed), true);
    seekerTransform->rot = atan2(arrow->dir.y, arrow->dir.x);
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
