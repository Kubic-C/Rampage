#pragma once
#include "utility/base.hpp"
#include "utility/ecs.hpp"
#include "transform.hpp"

struct BodyComponent {
  b2BodyId id = b2_nullBodyId;

  ~BodyComponent() {
    b2DestroyBody(id);
  }
};

inline void* entityToB2Data(EntityId id) {
  return (void*)id;
}

inline Entity b2DataToEntity(EntityWorld& world, void* vp) {
  return world.get((EntityId)vp);
}

inline void copyTransformsIntoBodies(TransformComponent& transform, BodyComponent& body) {
  if (!b2Body_IsValid(body.id))
    return;

  /* SetTransform is expensive to call, only set the transform of the body if transform does not match*/
  if (transform != b2Body_GetTransform(body.id)) {
    b2Body_SetTransform(body.id, transform.pos, transform.rot);
  }
}

inline void copyBodiesIntoTransforms(TransformComponent& transform, BodyComponent& body) {
  if (!b2Body_IsValid(body.id))
    return;

  b2Transform bodyTransform = b2Body_GetTransform(body.id);
  if (transform != bodyTransform) {
    transform.pos = bodyTransform.p;
    transform.rot = bodyTransform.q;
  }
}

struct CollisionQueueComponent {
  struct Collision {
    EntityId primary; // The entity which "made" the submission
    EntityId secondary; // The other entity
    Vec2 manifold;
  };

  std::vector<Collision> queue;
};

struct SubmitToCollisionQueueComponent {
  EntityId queue;
};

struct PhysicsModule : Module {
  PhysicsModule(EntityWorld& world, int steps)
    : m_steps(steps) {
    world.component<BodyComponent>();
    world.component<TransformComponent>();
    world.component<CollisionQueueComponent>();
    world.component<SubmitToCollisionQueueComponent>();
  }

  void run(EntityWorld& world, float deltaTime) override final {
    EntityIterator iter = world.getWith(world.set<TransformComponent, BodyComponent>());
    b2WorldId physicsWorldId = world.getContext<b2WorldId>();

    while (iter.hasNext()) {
      Entity entity = iter.next();

      copyTransformsIntoBodies(entity.get<TransformComponent>(), entity.get<BodyComponent>());
    }

    b2World_Step(physicsWorldId, deltaTime, m_steps);

    iter.reset();
    while (iter.hasNext()) {
      Entity entity = iter.next();

      copyBodiesIntoTransforms(entity.get<TransformComponent>(), entity.get<BodyComponent>());
    }

    b2ContactEvents events = b2World_GetContactEvents(physicsWorldId);
    for (int i = 0; i < events.hitCount; i++) {
      const b2ContactHitEvent& hit = events.hitEvents[i];
      
      void* Ab2Data = b2Body_GetUserData(b2Shape_GetBody(hit.shapeIdA));
      void* Bb2Data = b2Body_GetUserData(b2Shape_GetBody(hit.shapeIdB));
      if (!Ab2Data || !Bb2Data)
        continue;

      Entity eA = b2DataToEntity(world, Ab2Data);
      Entity eB = b2DataToEntity(world, Bb2Data);
      CollisionQueueComponent::Collision collision;
      collision.manifold = hit.point;
      collision.primary = eA;
      collision.secondary = eB;
      if (eA.has<SubmitToCollisionQueueComponent>()) {
        SubmitToCollisionQueueComponent& submitTo = eA.get<SubmitToCollisionQueueComponent>();
        CollisionQueueComponent& queue = world.get(submitTo.queue).get<CollisionQueueComponent>();

        queue.queue.push_back(collision);
      }
      std::swap(collision.primary, collision.secondary);
      if (eB.has<SubmitToCollisionQueueComponent>()) {
        SubmitToCollisionQueueComponent& submitTo = eB.get<SubmitToCollisionQueueComponent>();
        CollisionQueueComponent& queue = world.get(submitTo.queue).get<CollisionQueueComponent>();

        queue.queue.push_back(collision);
      }
    }
  }

private:
  int m_steps;
};