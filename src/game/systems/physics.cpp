#include "physics.hpp"

#include "../../core/module.hpp"
#include "../module.hpp"

#include "../components/body.hpp"
#include "../components/collisionQueue.hpp"

RAMPAGE_START

struct PhysicsContext {
  int steps = 0;
  std::vector<EntityId> sparseShapeEntity;
  Map<EntityId, std::set<EntityId>> ongoingCollisions;

  void insertShapeEntity(b2ShapeId shapeId, EntityId entity) {
    int index = shapeId.index1;

    if (index >= sparseShapeEntity.size())
      sparseShapeEntity.resize(index + 1, 0);

    sparseShapeEntity[index] = entity;
  }
};

int copyTransformsIntoBodies(EntityWorld& world, float dt) {
  auto it = world.getWith(world.set<TransformComponent, BodyComponent>());
  while (it.hasNext()) {
    Entity e = it.next();
    auto transform = e.get<TransformComponent>();
    auto body = e.get<BodyComponent>();

    if (!b2Body_IsValid(body->id))
      continue;

    /* SetTransform is expensive to call, only set the transform of the body if
     * transform does not match*/
    if (*transform != b2Body_GetTransform(body->id)) {
      b2Body_SetTransform(body->id, transform->pos, transform->rot);
    }
  }

  return 0;
}

int copyBodiesIntoTransforms(EntityWorld& world, float dt) {
  auto it = world.getWith(world.set<TransformComponent, BodyComponent>());
  while (it.hasNext()) {
    Entity e = it.next();
    auto transform = e.get<TransformComponent>();
    auto body = e.get<BodyComponent>();
    if (!b2Body_IsValid(body->id))
      continue;

    b2Transform bodyTransform = b2Body_GetTransform(body->id);
    if (*transform != bodyTransform) {
      transform->pos = bodyTransform.p;
      transform->rot = bodyTransform.q;
    }
  }

  return 0;
}

int physicsStep(EntityWorld& world, float dt) {
  EntityIterator iter = world.getWith(world.set<TransformComponent, BodyComponent>());
  b2WorldId physicsWorldId = world.getContext<b2WorldId>();
  PhysicsContext& context = world.getContext<PhysicsContext>();

  b2World_Step(physicsWorldId, dt, context.steps);

  b2ContactEvents events = b2World_GetContactEvents(physicsWorldId);
  for (int i = 0; i < events.beginCount; i++) {
    const b2ContactBeginTouchEvent& touch = events.beginEvents[i];

    void* Ab2Data = b2Shape_GetUserData(touch.shapeIdA);
    void* Bb2Data = b2Shape_GetUserData(touch.shapeIdB);
    if (!Ab2Data || !Bb2Data)
      continue;

    Entity eA = b2DataToEntity(world, Ab2Data);
    Entity eB = b2DataToEntity(world, Bb2Data);
    context.ongoingCollisions[eA].insert(eB);
    context.ongoingCollisions[eB].insert(eA);
    context.insertShapeEntity(touch.shapeIdA, eA);
    context.insertShapeEntity(touch.shapeIdB, eB);
  }
  for (int i = 0; i < events.endCount; i++) {
    const b2ContactEndTouchEvent& touch = events.endEvents[i];

    if (touch.shapeIdA.index1 > context.sparseShapeEntity.size() ||
        touch.shapeIdB.index1 > context.sparseShapeEntity.size())
      continue;

    EntityId eA = context.sparseShapeEntity[touch.shapeIdA.index1];
    EntityId eB = context.sparseShapeEntity[touch.shapeIdB.index1];
    if (eA == NullEntityId || eB == NullEntityId)
      continue;

    context.ongoingCollisions[eA].erase(eB);
    context.ongoingCollisions[eB].erase(eA);
    if (context.ongoingCollisions[eA].empty())
      context.ongoingCollisions.erase(eA);
    if (context.ongoingCollisions[eB].empty())
      context.ongoingCollisions.erase(eB);
  }

  for (auto& [entity, contactList] : context.ongoingCollisions) {
    Entity eA = world.get(entity);

    for (EntityId other : contactList) {
      Entity eB = world.get(other);

      CollisionQueueComponent::Collision collision;
      collision.primary = eA;
      collision.secondary = eB;
      if (eA.has<SubmitToCollisionQueueComponent>()) {
        auto submitTo = eA.get<SubmitToCollisionQueueComponent>();
        auto queue = world.get(submitTo->queue).get<CollisionQueueComponent>();

        queue->queue.push_back(collision);
      }
      std::swap(collision.primary, collision.secondary);
      if (eB.has<SubmitToCollisionQueueComponent>()) {
        auto submitTo = eB.get<SubmitToCollisionQueueComponent>();
        auto queue = world.get(submitTo->queue).get<CollisionQueueComponent>();

        queue->queue.push_back(collision);
      }
    }
  }

  world.beginDefer();
  auto it = world.getWith(world.set<AddBodyComponent>());
  while (it.hasNext()) {
    Entity e = it.next();

    e.add<BodyComponent>();
    RefT<BodyComponent> bodyId = e.get<BodyComponent>();
    if (b2Body_IsValid(bodyId->id))
      b2DestroyBody(bodyId->id);

    RefT<AddBodyComponent> bodyDef = e.get<AddBodyComponent>();
    bodyDef->userData = entityToB2Data(e);
    if (e.has<TransformComponent>()) {
      RefT<TransformComponent> transform = e.get<TransformComponent>();
      bodyDef->position = transform->pos;
      bodyDef->rotation = transform->rot;
    }

    bodyId->id = b2CreateBody(world.getContext<b2WorldId>(), &*bodyDef);

    e.remove<AddBodyComponent>();
  }

  it = world.getWith(world.set<AddShapeComponent>());
  while (it.hasNext()) {
    Entity e = it.next();

    if (!e.has<BodyComponent>())
      throw std::runtime_error("Error: cannot add shape, body not created");

    auto bodyId = e.get<BodyComponent>();
    auto shapeDef = e.get<AddShapeComponent>();
    shapeDef->def.userData = entityToB2Data(e);
    if (shapeDef->shape.index() == IndexInVariant<ShapeVariant, b2Circle>)
      b2CreateCircleShape(bodyId->id, &shapeDef->def, &std::get<b2Circle>(shapeDef->shape));
    else if (shapeDef->shape.index() == IndexInVariant<ShapeVariant, b2Polygon>)
      b2CreatePolygonShape(bodyId->id, &shapeDef->def, &std::get<b2Polygon>(shapeDef->shape));
    else
      throw std::runtime_error("Error: cannot add shape, unknown shape type");

    e.remove<AddShapeComponent>();
  }
  world.endDefer();
}

struct PrePhysicsStepStage {};
struct PhysicsStepStage {};
struct PostPhysicsStepStage {};

void loadPhysicsSystems(IHost& host, int steps) {
  Pipeline& pipeline = host.getPipeline();
  EntityWorld& world = host.getWorld();

  world.addContext<PhysicsContext>();
  auto& context = world.getContext<PhysicsContext>();
  context.steps = steps;

  pipeline.getGroup<GameGroup>()
      .createStageAfter<GameGroup::TickStage, PrePhysicsStepStage>()
      .createStageAfter<PrePhysicsStepStage, PhysicsStepStage>()
      .createStageAfter<PhysicsStepStage, PostPhysicsStepStage>()
      .attachToStage<PrePhysicsStepStage>(copyTransformsIntoBodies)
      .attachToStage<PhysicsStepStage>(physicsStep)
      .attachToStage<PostPhysicsStepStage>(copyBodiesIntoTransforms);
}

RAMPAGE_END
