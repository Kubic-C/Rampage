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

    EntityBox2dUserData* Ab2Data = getEntityBox2dUserData(touch.shapeIdA);
    EntityBox2dUserData* Bb2Data = getEntityBox2dUserData(touch.shapeIdB);
    if (!Ab2Data || !Bb2Data)
      continue;
    EntityPtr eA = world->getEntity(Ab2Data->entity);
    EntityPtr eB = world->getEntity(Bb2Data->entity);

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

    EntityBox2dUserData* Ab2Data = getEntityBox2dUserData(touch.shapeIdA);
    EntityBox2dUserData* Bb2Data = getEntityBox2dUserData(touch.shapeIdB);
    if (!Ab2Data || !Bb2Data)
      continue;
    EntityPtr eA = world->getEntity(Ab2Data->entity);
    EntityPtr eB = world->getEntity(Bb2Data->entity);

    eA.add<LastCollisionData>();
    eB.add<LastCollisionData>();
    eA.get<LastCollisionData>()->other = eB;
    eB.get<LastCollisionData>()->other = eA;

    world->emit<OnCollisionEndEvent>(eA, world->component<LastCollisionData>());
    world->emit<OnCollisionEndEvent>(eB, world->component<LastCollisionData>());
  }

  return 0;
}

struct PrePhysicsStepStage {};
struct PhysicsStepStage {};
struct PostPhysicsStepStage {};

void observeDestroyedBody(EntityPtr entity) {
  IWorldPtr world = entity.world();
  auto bodyId = entity.get<BodyComponent>()->id;
  auto lastCollisionData = entity.get<LastCollisionData>();

  if(!b2Body_IsValid(bodyId))
    return;

  std::vector<b2ContactData> contacts(b2Body_GetContactCapacity(bodyId));
  int actualSize = b2Body_GetContactData(bodyId, contacts.data(), contacts.size());
  contacts.resize(actualSize);

  for (const b2ContactData& contactData : contacts) {
    EntityBox2dUserData* other = getEntityBox2dUserData(contactData.shapeIdA);
    if (!other || other->entity == entity)
        other = getEntityBox2dUserData(contactData.shapeIdB);
    if (!other)
      continue;

    lastCollisionData->other = other->entity;
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

  world->observe<ComponentRemovedEvent>(world->component<BodyComponent>(), world->set<LastCollisionData>(), observeDestroyedBody);
}

RAMPAGE_END
