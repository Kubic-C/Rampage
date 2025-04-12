#pragma once
#include "utility/base.hpp"
#include "utility/ecs.hpp"
#include "transform.hpp"

struct BodyComponent {
  b2BodyId id = b2_nullBodyId;
};

inline void copyTransformsIntoBodies(PosComponent& pos, RotComponent& rot, BodyComponent& body) {
  if (!b2Body_IsValid(body.id))
    return;

  /* SetTransform is expensive to call, only set the transform of the body if transform does not match*/
  if (Transform(pos, rot) != b2Body_GetTransform(body.id)) {
    b2Body_SetTransform(body.id, pos, rot);
  }
}

inline void copyBodiesIntoTransforms(PosComponent& pos, RotComponent& rot, BodyComponent& body) {
  if (!b2Body_IsValid(body.id))
    return;

  b2Transform bodyTransform = b2Body_GetTransform(body.id);
  if (Transform(pos, rot) != bodyTransform) {
    pos = bodyTransform.p;
    rot = bodyTransform.q;
  }
}

struct PhysicsModule : Module {
  PhysicsModule(EntityWorld& world, int steps)
    : m_steps(steps) {
    world.component<BodyComponent>();
    world.component<PosComponent>();
    world.component<RotComponent>();
  }

  void run(EntityWorld& world, float deltaTime) override final {
    EntityIterator iter = world.getWith(world.set<PosComponent, RotComponent, BodyComponent>());
    b2WorldId physicsWorldId = world.getContext<b2WorldId>();

    while (iter.hasNext()) {
      Entity entity = iter.next();

      copyTransformsIntoBodies(entity.get<PosComponent>(), entity.get<RotComponent>(), entity.get<BodyComponent>());
    }

    b2World_Step(physicsWorldId, deltaTime, m_steps);

    iter.reset();
    while (iter.hasNext()) {
      Entity entity = iter.next();

      copyBodiesIntoTransforms(entity.get<PosComponent>(), entity.get<RotComponent>(), entity.get<BodyComponent>());
    }
  }

private:
  int m_steps;
};