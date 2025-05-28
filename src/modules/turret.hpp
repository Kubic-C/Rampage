#pragma once

#include "../components/health.hpp"
#include "../components/transform.hpp"
#include "../components/turret.hpp"

class TurretModule : public Module {
  struct ClosestShape {
    b2ShapeId shape = b2_nullShapeId;
    float dist = std::numeric_limits<float>::max();
    b2Vec2 center;
  };

  struct SummonBullet {
    EntityId id;
    Vec2 shootVelocity;
    Vec2 pos;
    float radius = 0.25f;
    float health;
    float damage;
  };

  static bool queryClosest(b2ShapeId shape, void* ctx) {
    ClosestShape& closest = *(ClosestShape*)ctx;

    float dist = b2Distance(b2Body_GetPosition(b2Shape_GetBody(shape)), closest.center);
    if (dist < closest.dist) {
      closest.shape = shape;
      closest.dist = dist;
    }

    return true;
  }

  static float signedAngleDiff(float a, float b) {
    constexpr float pi = glm::pi<float>();
    float diff = glm::mod(b - a, 2 * pi);
    if (diff > pi)
      diff -= 2 * pi;
    return diff;
  }

  const Vec2 right = {1.0f, 0.0f};

  public:
  static void registerComponents(EntityWorld& world) { world.component<TurretComponent>(); }

  TurretModule(EntityWorld& world) :
      m_turretSys(world.system(
          world.set<TransformComponent, TurretComponent>(),
          std::bind(&TurretModule::turretSystem, this, std::placeholders::_1, std::placeholders::_2))),
      m_collisionQueue(world.create()) {
    m_collisionQueue.add<CollisionQueueComponent>();

    world.observe(EntityWorld::EventType::Add, world.component<BulletDamageComponent>(),
                  world.set<EntityWorld::Enabled>(), [&](Entity e) {
                    e.add<SubmitToCollisionQueueComponent>();
                    RefT<SubmitToCollisionQueueComponent> queue = e.get<SubmitToCollisionQueueComponent>();
                    queue->queue = m_collisionQueue;
                  });
  }

  void turretSystem(Entity e, float dt) {
    b2WorldId physicsWorld = e.world().getContext<b2WorldId>();
    RefT<TransformComponent> transform = e.get<TransformComponent>();
    RefT<TurretComponent> turret = e.get<TurretComponent>();
    RefT<SpriteComponent> sprite = e.get<SpriteComponent>();

    ClosestShape closestShape;
    closestShape.center = transform->pos;
    b2ShapeProxy proxy;
    proxy.count = 1;
    proxy.points[0] = transform->pos;
    proxy.radius = turret->radius;
    b2QueryFilter filter;
    filter.maskBits = Enemy;
    filter.categoryBits = All;
    b2World_OverlapShape(physicsWorld, &proxy, filter, &queryClosest, &closestShape);
    if (!b2Shape_IsValid(closestShape.shape))
      return;

    b2BodyId targetBody = b2Shape_GetBody(closestShape.shape);
    Vec2 targetPos = b2Body_GetPosition(targetBody);
    Vec2 targetDir = glm::normalize(targetPos - transform->pos);
    float targetRot = atan2f(targetDir.y, targetDir.x);

    float closestDistToRotate = signedAngleDiff(turret->rot, targetRot);
    float distToRotate = glm::abs(closestDistToRotate);
    if (distToRotate < turret->shootRange) {
      turret->timeSinceLastShot += dt;

      if (turret->timeSinceLastShot >= turret->fireRate) {
        SummonBullet bullet;
        bullet.id = turret->summon;
        bullet.pos = transform->pos;
        bullet.shootVelocity = fast2DRotate(right, turret->rot) * turret->muzzleVelocity;
        bullet.radius = turret->bulletRadius;
        bullet.health = turret->bulletHealth;
        bullet.damage = turret->bulletDamage;
        m_summonBullets.push_back(bullet);

        turret->timeSinceLastShot = 0.0f;
      }
    }

    if (distToRotate > turret->stopRange) {
      float dir = turret->turnSpeed;
      if (closestDistToRotate < 0)
        dir = -dir;

      if (distToRotate < turret->turnSpeed)
        turret->rot = targetRot;
      else
        turret->rot += dir;
    }

    sprite->getLast().rot = turret->rot;
  }

  void run(EntityWorld& world, float deltaTime) override {
    RefT<CollisionQueueComponent> queue = m_collisionQueue.get<CollisionQueueComponent>();
    for (const CollisionQueueComponent::Collision& collision : queue->queue) {
      // its possible for a bullet to be destroyed by the HealthModule before we
      // can get to it.
      if (!world.isAlive(collision.primary)) {
        logGeneric("Bullet not alive, skipping!\n");
        continue;
      }
      Entity bullet = world.get(collision.primary);
      // its possible for a enemy to be destroyed by the HealthModule before we
      // can get to it.
      if (!world.isAlive(collision.secondary)) {
        logGeneric("Zombie not alive, skipping!\n");
        continue;
      }
      Entity other = world.get(collision.secondary);

      assert(other.has<HealthComponent>());

      RefT<HealthComponent> health = other.get<HealthComponent>();
      RefT<BulletDamageComponent> damage = bullet.get<BulletDamageComponent>();
      health->health -= damage->damage;

      RefT<HealthComponent> bulletHealth = bullet.get<HealthComponent>();
      bulletHealth->health -= damage->damage * 0.1f;

      logGeneric("Shot: %i\n", collision.primary);
    }
    queue->queue.clear();

    m_turretSys.run(deltaTime);

    b2WorldId physicsWorld = world.getContext<b2WorldId>();
    for (SummonBullet& bullet : m_summonBullets) {
      Entity bulletEntity = world.create();
      bulletEntity.add(world.set<LifetimeComponent, HealthComponent, BulletDamageComponent, BodyComponent,
                                 CircleRenderComponent, TransformComponent>());

      bulletEntity.get<TransformComponent>()->pos = bullet.pos;

      RefT<CircleRenderComponent> cirlceRender = bulletEntity.get<CircleRenderComponent>();
      cirlceRender->radius = bullet.radius;
      cirlceRender->color = glm::vec3(1.0f, 1.0f, 0.0f);

      bulletEntity.get<HealthComponent>()->health = bullet.health;
      bulletEntity.get<BulletDamageComponent>()->damage = bullet.damage;

      RefT<BodyComponent> bodyComp = bulletEntity.get<BodyComponent>();
      b2BodyDef bodyDef = b2DefaultBodyDef();
      bodyDef.type = b2_dynamicBody;
      bodyDef.linearVelocity = bullet.shootVelocity;
      bodyDef.position = bullet.pos;
      bodyDef.userData = entityToB2Data(bulletEntity);
      bodyComp->id = b2CreateBody(physicsWorld, &bodyDef);
      b2Circle circle;
      circle.center = Vec2(0);
      circle.radius = bullet.radius;
      b2ShapeDef shapeDef = b2DefaultShapeDef();
      shapeDef.density = 100.0f;
      shapeDef.enableContactEvents = true;
      shapeDef.userData = entityToB2Data(bulletEntity);
      b2Filter filter;
      filter.categoryBits = Friendly;
      filter.maskBits = Enemy;
      filter.groupIndex = 0;
      shapeDef.filter = filter;
      b2CreateCircleShape(bodyComp->id, &shapeDef, &circle);
    }
    m_summonBullets.clear();
  }

  private:
  System m_turretSys;
  std::vector<SummonBullet> m_summonBullets;
  Entity m_collisionQueue;
};
