#pragma once

#include "../components/transform.hpp"
#include "../components/body.hpp"
#include "../components/collisionQueue.hpp"

inline void copyTransformsIntoBodies(RefT<TransformComponent> transform, RefT<BodyComponent> body) {
  if (!b2Body_IsValid(body->id))
    return;

  /* SetTransform is expensive to call, only set the transform of the body if transform does not match*/
  if (*transform != b2Body_GetTransform(body->id)) {
    b2Body_SetTransform(body->id, transform->pos, transform->rot);
  }
}

inline void copyBodiesIntoTransforms(RefT<TransformComponent> transform, RefT<BodyComponent> body) {
  if (!b2Body_IsValid(body->id))
    return;

  b2Transform bodyTransform = b2Body_GetTransform(body->id);
  if (*transform != bodyTransform) {
    transform->pos = bodyTransform.p;
    transform->rot = bodyTransform.q;
  }
}

struct PhysicsModule : Module {
private:
  inline static bool m_created = false;
public:

  static void registerComponents(EntityWorld& world) {
    world.component<BodyComponent>();
    world.component<TransformComponent>();
    world.component<CollisionQueueComponent>();
    world.component<SubmitToCollisionQueueComponent>();
  }

  PhysicsModule(EntityWorld& world, int steps)
    : m_steps(steps) {
    if (m_created) {
      logGeneric("PhysicsModule can not be instanced twice\n");
      throw std::runtime_error("Error: double instanced");
    }
    m_created = true;

    world.component<AddBodyComponent>();
    world.component<AddShapeComponent>();

    world.observe(EntityWorld::EventType::Remove, world.component<BodyComponent>(), {},
      [&](Entity e) {
        auto it = m_ongoingCollisions.find(e);
        if (it != m_ongoingCollisions.end()) {
          for (EntityId other : it->second) {
            m_ongoingCollisions[other].erase(e);
            if (m_ongoingCollisions[other].empty())
              m_ongoingCollisions.erase(other);
          }
          m_ongoingCollisions.erase(it);
          if (e.has<SubmitToCollisionQueueComponent>())
            e.world().get(e.get<SubmitToCollisionQueueComponent>()->queue).get<CollisionQueueComponent>()->queue.clear();
        }
      });
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
    for (int i = 0; i < events.beginCount; i++) {
      const b2ContactBeginTouchEvent& touch = events.beginEvents[i]; 
      
      void* Ab2Data = b2Shape_GetUserData(touch.shapeIdA);
      void* Bb2Data = b2Shape_GetUserData(touch.shapeIdB);
      if (!Ab2Data || !Bb2Data)
        continue;

      Entity eA = b2DataToEntity(world, Ab2Data);
      Entity eB = b2DataToEntity(world, Bb2Data);
      m_ongoingCollisions[eA].insert(eB);
      m_ongoingCollisions[eB].insert(eA);
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

      Entity eA = b2DataToEntity(world, Ab2Data);
      Entity eB = b2DataToEntity(world, Bb2Data);
      m_ongoingCollisions[eA].erase(eB);
      m_ongoingCollisions[eB].erase(eA);
      if (m_ongoingCollisions[eA].empty())
        m_ongoingCollisions.erase(eA);
      if (m_ongoingCollisions[eB].empty())
        m_ongoingCollisions.erase(eB);
    }

    for (auto& [entity, contactList] : m_ongoingCollisions) {
      Entity eA = world.get(entity);

      for (EntityId other : contactList) {
        Entity eB = world.get(other);

        CollisionQueueComponent::Collision collision;
        collision.primary = eA;
        collision.secondary = eB;
        if (eA.has<SubmitToCollisionQueueComponent>()) {
          RefT<SubmitToCollisionQueueComponent> submitTo = eA.get<SubmitToCollisionQueueComponent>();
          RefT<CollisionQueueComponent> queue = world.get(submitTo->queue).get<CollisionQueueComponent>();

          queue->queue.push_back(collision);
        }
        std::swap(collision.primary, collision.secondary);
        if (eB.has<SubmitToCollisionQueueComponent>()) {
          RefT<SubmitToCollisionQueueComponent> submitTo = eB.get<SubmitToCollisionQueueComponent>();
          RefT<CollisionQueueComponent> queue = world.get(submitTo->queue).get<CollisionQueueComponent>();

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

      RefT<BodyComponent> bodyId = e.get<BodyComponent>();
      RefT<AddShapeComponent> shapeDef = e.get<AddShapeComponent>();
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

  Map<EntityId, std::set<EntityId>>& getOngoingCollisions() {
    return m_ongoingCollisions;
  }

private:
  int m_steps;

  Map<EntityId, std::set<EntityId>> m_ongoingCollisions;
};