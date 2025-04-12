#pragma once

#include "utility/base.hpp"
#include "render/render.hpp"
#include "utility/log.hpp"
#include "tilemap.hpp"
#include "player.hpp"
#include "physics.hpp"
#include "seekPlayer.hpp"
#include "worldMap.hpp"

class App {
public:
  App(const std::string_view& appName, float ticksPerSecond)
    : m_player(m_world.create()), m_localAppStatus(Status::Ok), m_ticksPerSecond(ticksPerSecond) {
    /* Physics setup */
    b2WorldDef physicsWorldDef = b2DefaultWorldDef();
    physicsWorldDef.gravity = b2Vec2{ 0 };
    m_physicsWorld = b2CreateWorld(&physicsWorldDef);

    m_world.addContext<b2WorldId>(m_physicsWorld);
    m_world.addModule<PhysicsModule>(4);

    /* Window and render setup */
    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, Render::MinimumMajorGLVersion);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, Render::MinimumMinorGLVersion);
    m_window = SDL_CreateWindow(appName.data(), 800, 600, Render::getNecessaryWindowFlags());
    if (!m_window) {
      m_localAppStatus = Status::CriticalError;
      logError(1, "Failed to create window. SDL_GetError(): %s\n", SDL_GetError());
      return;
    }

    m_world.addContext<SDL_Window*>(m_window);

    m_context = SDL_GL_CreateContext(m_window);
    if (!m_context) {
      m_localAppStatus = Status::CriticalError;
      logError(1, "Failed to create context. SDL_GetError(): %s\n", SDL_GetError());
      return;
    }

    m_player.add(m_world.set<PosComponent, RotComponent, CameraComponent>());
    m_player.get<PosComponent>() = { 0 };
    m_player.get<RotComponent>() = { 0 };
    m_player.get<CameraComponent>().m_zoom = 100.0f;

    m_render = new Render(m_window, m_player);
    if (m_render->getStatus() == Status::CriticalError) {
      logError(1, "Failed to create render. SDL_GetError(): %s\n", SDL_GetError());
      m_localAppStatus = Status::CriticalError;
      return;
    }

    m_world.addContext<Render*>(m_render);

    /* Tile drawing ... */
    if (!m_render->loadSprite(0, "./res/unknown.png")) {
      logError(1, "Failed to load resource.\n");
      m_localAppStatus = Status::CriticalError;
      return;
    }

    if (!m_render->loadSprite(1, "./res/stone.png")) {
      logError(1, "Failed to load resource.\n");
      m_localAppStatus = Status::CriticalError;
      return;
    }

    /* Player Component ... */
    m_world.addModule<PlayerModule>();
    m_player.add<PlayerComponent>();

    m_player.add<BodyComponent>();
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.fixedRotation = true;
    bodyDef.linearDamping = 10;
    b2BodyId bodyId = b2CreateBody(m_physicsWorld, &bodyDef);
    m_player.get<BodyComponent>().id = bodyId;
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1000;
    b2Polygon rect = b2MakeBox(0.12f, 0.12f);
    b2CreatePolygonShape(bodyId, &shapeDef, &rect);

    m_player.get<PosComponent>() = { -1.0f, 0.0f };

    /* Tilemap Component */
    m_world.component<TilemapComponent>();
    m_world.component<SpriteComponent>();
    
    { // World Map
      Entity tm = m_world.create();
      tm.add<PosComponent>();
      tm.add<RotComponent>();
      tm.add<BodyComponent>();
      tm.add<TilemapComponent>();
      tm.add<WorldMapTag>();

      tm.get<PosComponent>() = { 0, 0 };
      tm.get<RotComponent>() = Rot(0);
      b2BodyDef bodyDef = b2DefaultBodyDef();
      bodyDef.type = b2_staticBody;
      bodyDef.position = Vec2(0, 0);
      b2BodyId bodyId = b2CreateBody(m_physicsWorld, &bodyDef);
      tm.get<BodyComponent>().id = bodyId;

      TilemapComponent& tilemap = tm.get<TilemapComponent>();
      auto tileCallback =
        [&](int x, int y) {
          Entity e = m_world.create();
          e.add<SpriteComponent>();
          e.add<ArrowComponent>();
          e.get<SpriteComponent>().texIndex = 1;

          if (x <= -90 || x >= 90 || y <= -90 || y >= 90) {
            e.get<SpriteComponent>().texIndex = 0;
            tilemap.insert({ x, y }, bodyId, e, TileFlags::IS_COLLIDABLE);
          }
          else {
            tilemap.insert({ x, y }, bodyId, e, 0);
          }
        };

      callInGrid(-100, -100, 100, 100, tileCallback);
    }

    auto bodyCallback =
      [&](int x, int y) {
        Entity tm = m_world.create();
        tm.add<PosComponent>();
        tm.add<RotComponent>();
        tm.add<BodyComponent>();
        tm.add<TilemapComponent>();

        tm.get<PosComponent>() = { x * 0.3f, y * 0.3f };
        tm.get<RotComponent>() = Rot(0);
        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type = b2_dynamicBody;
        bodyDef.position = Vec2(0, 0);
        tm.get<BodyComponent>().id = b2CreateBody(m_physicsWorld, &bodyDef);

        TilemapComponent& tilemap = tm.get<TilemapComponent>();
        tilemap.insert({ 0, 0 }, tm.get<BodyComponent>().id);

        b2Body_ApplyAngularImpulse(tm.get<BodyComponent>().id, 100, true);
      };

    callInGrid(-40, -40, 40, 40, bodyCallback);
  }

  Status getStatus() {
    return m_localAppStatus;
  }

  float now() {
    auto current_time = std::chrono::high_resolution_clock::now();

    return (float)std::chrono::duration_cast<std::chrono::microseconds>(current_time - start_time).count() / (1000000);
  }

  void tick(u32 tick, float deltaTime) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
      case SDL_EVENT_QUIT:
        m_exit = true;
        break;
      case SDL_EVENT_WINDOW_RESIZED:
        m_render->resizeViewportToScreenDim();
        break;
      }
    }

    PlayerComponent& player = m_player.get<PlayerComponent>();
    EntityIterator it = m_world.getWith(m_world.set<BodyComponent>());
    while (it.hasNext() && SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_F2]) {
      Entity e = it.next();

      if (e.has<PlayerComponent>())
        continue;
      b2BodyId bodyId = e.get<BodyComponent>().id;
      if(!b2Body_IsValid(bodyId))
        continue;

      glm::vec2 dir = (Vec2)b2Normalize(b2Sub((Vec2)player.mouse, b2Body_GetPosition(bodyId))) * deltaTime;

      if(b2Length(b2Body_GetLinearVelocity(bodyId)) < 100.0f)
        b2Body_ApplyLinearImpulseToCenter(bodyId, Vec2(dir), true);
    }

    m_world.run(deltaTime);
  }

  void update(float frameTime) {
    static float dt = 0;
    dt += frameTime;

    m_render->clear();

    /* Tilemap drawing */
    {
      EntityIterator it = m_world.getWith(m_world.set<TilemapComponent, PosComponent, RotComponent>());
      while (it.hasNext()) {
        Entity e = it.next();
        TilemapComponent& tm = e.get<TilemapComponent>();
        PosComponent& pos = e.get<PosComponent>();
        RotComponent& rot = e.get<RotComponent>();

        m_render->startTileBatch();
        for (glm::i16vec2 pos : tm) {
          Tile& tile = tm.find(pos);
          if (!(tile.flags & TileFlags::IS_MAIN_TILE))
            continue;

          u16 index = 0;
          if(tile.entity)
            index = m_world.get(tile.entity).get<SpriteComponent>().texIndex;

          m_render->addTile(index, pos, { tile.width, tile.height });
        }
        m_render->drawTileBatch(Transform(pos, rot));
      }
    }

    /* Player drawing */
    {
      EntityIterator it = m_world.getWith(m_world.set<PlayerComponent, PosComponent, RotComponent>());
      while (it.hasNext()) {
        Entity e = it.next();
        PlayerComponent& tm = e.get<PlayerComponent>();
        PosComponent& pos = e.get<PosComponent>();
        RotComponent& rot = e.get<RotComponent>();

        m_render->drawCircle(Transform(pos, rot), glm::vec3(1.0f, 0.5f, 0.25f), 0.1f, 6, 0);
      }
    }

    /* Debug physics drawing */
    if(SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_F1]) {
      PlayerComponent& player = m_player.get<PlayerComponent>();
      const int testRadius = 1.0f;
      const Vec2 rayPoint = player.mouse;

      m_render->drawHollowCircle(Transform(rayPoint, 0), glm::vec3(0.0f, 1.0f, 0.0f), testRadius);

      std::vector<b2BodyId> nearbyBodies;
      auto overlapClbk =
        [](b2ShapeId shapeId, void* context) -> bool {
          reinterpret_cast<std::vector<b2BodyId>*>(context)->push_back(b2Shape_GetBody(shapeId));
          return true;
        };

      b2Circle circle;
      circle.radius = testRadius;
      circle.center = Vec2(0);
      b2World_OverlapCircle(m_physicsWorld, &circle, Transform(rayPoint, 0), b2DefaultQueryFilter(), overlapClbk, &nearbyBodies);
      for(b2BodyId& id : nearbyBodies) {
        std::vector<b2ShapeId> shapes;
        shapes.resize(b2Body_GetShapeCount(id));
        b2Body_GetShapes(id, shapes.data(), shapes.size());

        for (int i = 0; i < shapes.size(); i++) {
          b2ShapeId shape = shapes[i];

          b2ShapeType type = b2Shape_GetType(shape);
          switch (type) {
          case b2_polygonShape: {
            b2Polygon polygon = b2Shape_GetPolygon(shape);
            static std::vector<Vec2> points;
            points.clear();
            for (int i = 0; i < polygon.count; i++) {
              points.push_back(b2Body_GetWorldPoint(id, polygon.vertices[i]));
            }

            for (int i = 0; i < points.size(); i++) {
              Vec2 cur = points[i];
              Vec2 next = points[(i + 1) % points.size()];

              m_render->drawLine(cur, next, glm::vec3(1.0f, 0.0f, 0.0f), 0.009f);
            }
          } break;

          case b2_circleShape: {
            b2Circle circle = b2Shape_GetCircle(shape);

            m_render->drawCircle(b2Body_GetTransform(id), glm::vec3(1.0f, 0.0f, 0.0f), circle.radius, 20);
          } break;
          }
        }
      }
    }
       
    m_render->display();
  }

  int run() {
    float lastTime = 0;
    float ticksToGo = 0;

    float tickLastTime = 0;
    u32 tickI = 0;

    while (!m_exit) {
      float nowTime = now();
      float deltaTime = nowTime - lastTime;
      ticksToGo += deltaTime * m_ticksPerSecond;
      lastTime = nowTime;

      while (ticksToGo >= 1.0f) {
        float tickNowTime = now();
        float tickDeltaTime = tickNowTime - tickLastTime;
        tickLastTime = tickNowTime;

        tick(tickI, tickDeltaTime);

        ticksToGo--;
        tickI++;
      }

      update(deltaTime);
    }

    return 0;
  }

private:
  Status m_localAppStatus;
  b2WorldId m_physicsWorld;
  EntityWorld m_world;
  SDL_Window* m_window;
  SDL_GLContext m_context;
  Render* m_render;

  Entity m_player;

  float m_ticksPerSecond;
  bool m_exit = false;

  std::chrono::steady_clock::time_point start_time = std::chrono::high_resolution_clock::now();
};