#include "physics.hpp"

#include "../../core/module.hpp"
#include "../module.hpp"

#include "../components/body.hpp"
#include "../components/collisionEvent.hpp"

RAMPAGE_START

struct PhysicsContext {
  int steps = 0;
};

int copyTransformsIntoBodies(IWorldPtr world, float dt) {
  auto it = world->getWith(world->set<TransformComponent, BodyComponent>());
  while (it->hasNext()) {
    EntityPtr e = it->next();
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

int copyBodiesIntoTransforms(IWorldPtr world, float dt) {
  auto it = world->getWith(world->set<TransformComponent, BodyComponent>());
  while (it->hasNext()) {
    EntityPtr e = it->next();
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

int physicsStep(IWorldPtr world, float dt) {
  IEntityIteratorPtr iter = world->getWith(world->set<TransformComponent, BodyComponent>());
  b2WorldId physicsWorldId = world->getContext<b2WorldId>();
  PhysicsContext& context = world->getContext<PhysicsContext>();

  b2World_Step(physicsWorldId, dt, context.steps);

  b2ContactEvents events = b2World_GetContactEvents(physicsWorldId);
  for (int i = 0; i < events.beginCount; i++) {
    const b2ContactBeginTouchEvent& touch = events.beginEvents[i];

    void* Ab2Data = b2Shape_GetUserData(touch.shapeIdA);
    void* Bb2Data = b2Shape_GetUserData(touch.shapeIdB);
    if (!Ab2Data || !Bb2Data)
      continue;
    EntityPtr eA = b2DataToEntity(world, Ab2Data);
    EntityPtr eB = b2DataToEntity(world, Bb2Data);

    eA.add<LastCollisionData>();
    eB.add<LastCollisionData>();
    eA.get<LastCollisionData>()->other = eB;
    eB.get<LastCollisionData>()->other = eA;

    world->emit<OnCollisionBeginEvent>(eA, world->component<LastCollisionData>());
    world->emit<OnCollisionBeginEvent>(eB, world->component<LastCollisionData>());
  }
  for (int i = 0; i < events.endCount; i++) {
    const b2ContactEndTouchEvent& touch = events.endEvents[i];

    if (!b2Shape_IsValid(touch.shapeIdA) ||
        !b2Shape_IsValid(touch.shapeIdB))
      continue;

    void* Ab2Data = b2Shape_GetUserData(touch.shapeIdA);
    void* Bb2Data = b2Shape_GetUserData(touch.shapeIdB);
    if (!Ab2Data || !Bb2Data)
      continue;
    EntityPtr eA = b2DataToEntity(world, Ab2Data);
    EntityPtr eB = b2DataToEntity(world, Bb2Data);

    eA.add<LastCollisionData>();
    eB.add<LastCollisionData>();
    eA.get<LastCollisionData>()->other = eB;
    eB.get<LastCollisionData>()->other = eA;

    world->emit<OnCollisionEndEvent>(eA, world->component<LastCollisionData>());
    world->emit<OnCollisionEndEvent>(eB, world->component<LastCollisionData>());
  }

  world->beginDefer();
  auto it = world->getWith(world->set<AddBodyComponent>());
  while (it->hasNext()) {
    EntityPtr e = it->next();

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

    bodyId->id = b2CreateBody(world->getContext<b2WorldId>(), &*bodyDef);

    e.remove<AddBodyComponent>();
  }

  it = world->getWith(world->set<AddShapeComponent>());
  while (it->hasNext()) {
    EntityPtr e = it->next();

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
  world->endDefer();

  return 0;
}

struct PrePhysicsStepStage {};
struct PhysicsStepStage {};
struct PostPhysicsStepStage {};

void observeDestroyedBody(EntityPtr entity) {
  IWorldPtr world = entity.world();
  auto bodyId = entity.get<BodyComponent>()->id;
  auto lastCollisionData = entity.get<LastCollisionData>();

  std::vector<b2ContactData> contacts(b2Body_GetContactCapacity(bodyId));
  int actualSize = b2Body_GetContactData(bodyId, contacts.data(), contacts.size());
  contacts.resize(actualSize);

  for (const b2ContactData& contactData : contacts) {
    EntityId other = b2RawDataToEntity(b2Shape_GetUserData(contactData.shapeIdA));
    if (other == entity)
        other = b2RawDataToEntity(b2Shape_GetUserData(contactData.shapeIdB));
    if (other == NullEntityId)
      continue;

    lastCollisionData->other = other;
    world->emit<OnCollisionEndEvent>(entity, world->component<LastCollisionData>());
  }
}

void loadPhysicsSystems(IHost& host, int steps) {
  Pipeline& pipeline = host.getPipeline();
  IWorldPtr world = host.getWorld();

  world->addContext<PhysicsContext>();
  auto& context = world->getContext<PhysicsContext>();
  context.steps = steps;

  pipeline.getGroup<GameGroup>()
      .createStageBefore<GameGroup::TickStage, PrePhysicsStepStage>()
      .createStageAfter<PrePhysicsStepStage, PhysicsStepStage>()
      .createStageAfter<PhysicsStepStage, PostPhysicsStepStage>()
      .attachToStage<PrePhysicsStepStage>(copyTransformsIntoBodies)
      .attachToStage<PhysicsStepStage>(physicsStep)
      .attachToStage<PostPhysicsStepStage>(copyBodiesIntoTransforms);

  world->observe<ComponentRemovedEvent>(world->component<BodyComponent>(), {}, observeDestroyedBody);
}

RAMPAGE_END
