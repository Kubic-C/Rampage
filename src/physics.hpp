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

struct PhysicsModule : Module {
  PhysicsModule(EntityWorld& world, int steps)
    : m_steps(steps) {
    world.component<BodyComponent>();
    world.component<TransformComponent>();
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
  }

private:
  int m_steps;
};