#pragma once

#include "physics.hpp"
#include "tile.hpp"

enum PhysicsCategories {
  Friendly = 0x01,
  Enemy = 0x02,
  Static = 0x04,
  All = 0xFFFF
};

struct DamageComponent {
  float damage = 10.0f;
};

struct LifetimeComponent {
  float timeLeft = 1.0f;
};

struct TurretComponent {
  EntityId summon = 0;
  float fireRate = 1; // Per second
  float timeSinceLastShot = 0;
  float radius = 2.0f; // search radius

  float rot = 0; // When 0, the turret is facing right
  float turnSpeed = 0.1f;
  float shootRange = 0.04f; // + or -
  float stopRange = 0.02f;

  float muzzleVelocity = 10.0f;
  float bulletRadius = 0.25f;
};

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

  const Vec2 right = { 0.0f, 1.0f };
public:
  TurretModule(EntityWorld& world) 
    : m_tilemapSys(world.system(world.set<TransformComponent, TurretComponent>(), std::bind(&TurretModule::turretSystem, this, std::placeholders::_1, std::placeholders::_2))),
    m_shouldDieSys(world.system(world.set<LifetimeComponent>(), &shouldDieSystem)) {
    world.component<TurretComponent>();
    world.component<LifetimeComponent>();
    world.component<DamageComponent>();
  }

  static void shouldDieSystem(Entity e, float dt) {
    LifetimeComponent& destroyAfter = e.get<LifetimeComponent>();

    if (destroyAfter.timeLeft < 0)
      e.world().destroy(e);
    destroyAfter.timeLeft -= dt;
  }
     
  void turretSystem(Entity e, float dt) {
    b2WorldId physicsWorld = e.world().getContext<b2WorldId>();
    TransformComponent& transform = e.get<TransformComponent>();
    TurretComponent& turret = e.get<TurretComponent>();
    SpriteComponent& sprite = e.get<SpriteComponent>();
    
    ClosestShape closestShape;
    closestShape.center = transform.pos;
    b2Circle circle;
    circle.center = Vec2(0);
    circle.radius = turret.radius;
    b2QueryFilter filter;
    filter.maskBits = Enemy; 
    filter.categoryBits = All;
    b2World_OverlapCircle(physicsWorld, &circle, Transform(transform.pos, 0), filter, &queryClosest, &closestShape);
    if (!b2Shape_IsValid(closestShape.shape))
      return;

    Vec2 targetPos = b2Body_GetPosition(b2Shape_GetBody(closestShape.shape));
    Vec2 targetDir = glm::normalize(targetPos - transform.pos);
    float targetRot = atan2f(targetDir.y, targetDir.x);

    float closestDistToRotate = signedAngleDiff(turret.rot, targetRot);
    float distToRotate = glm::abs(closestDistToRotate);
    if (distToRotate < turret.shootRange) {
      turret.timeSinceLastShot += dt;

      if (turret.timeSinceLastShot >= turret.fireRate) {
        SummonBullet bullet;
        bullet.id = turret.summon;
        bullet.pos = transform.pos;
        bullet.shootVelocity = targetDir * turret.muzzleVelocity;
        bullet.radius = turret.bulletRadius;
        m_summonBullets.push_back(bullet);

        turret.timeSinceLastShot = 0.0f;
      }
    }

    if (distToRotate > turret.stopRange) {
      float dir = turret.turnSpeed;
      if (closestDistToRotate < 0)
        dir = -dir;

      if (distToRotate < turret.turnSpeed)
        turret.rot = targetRot;
      else
        turret.rot += dir;
    }

    sprite.getLast().rot = turret.rot;
  }

  void run(EntityWorld& world, float deltaTime) override {
    m_tilemapSys.run(deltaTime);
    m_shouldDieSys.run(deltaTime);

    b2WorldId physicsWorld = world.getContext<b2WorldId>();
    for (SummonBullet& bullet : m_summonBullets) {
      logGeneric("Created bullet\n");
      Entity bulletEntity = world.get(bullet.id).clone();
      bulletEntity.get<TransformComponent>().pos = bullet.pos;
      bulletEntity.add<BodyComponent>();
      b2BodyId& id = bulletEntity.get<BodyComponent>().id;

      bulletEntity.add<CircleRenderComponent>();
      CircleRenderComponent& cirlceRender = bulletEntity.get<CircleRenderComponent>();
      cirlceRender.radius = bullet.radius;

      b2BodyDef bodyDef = b2DefaultBodyDef();
      bodyDef.type = b2_dynamicBody;
      bodyDef.linearVelocity = bullet.shootVelocity;
      bodyDef.position = bullet.pos;
      id = b2CreateBody(physicsWorld, &bodyDef);
      b2Circle circle;
      circle.center = Vec2(0);
      circle.radius = bullet.radius;
      b2ShapeDef shapeDef = b2DefaultShapeDef();
      shapeDef.density = 100.0f;
      b2Filter filter;
      filter.categoryBits = Friendly;
      filter.maskBits = Enemy;
      filter.groupIndex = 0;
      shapeDef.filter = filter;
      b2CreateCircleShape(id, &shapeDef, &circle);
    }
    m_summonBullets.clear();
  }
private:
  System m_tilemapSys;
  System m_shouldDieSys;
  std::vector<SummonBullet> m_summonBullets;
};