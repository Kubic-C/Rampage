#pragma once

#include "utility/base.hpp"
#include "render/render.hpp"
#include "render/shapes.hpp"
#include "utility/log.hpp"
#include "tilemap.hpp"
#include "player.hpp"
#include "physics.hpp"
#include "seekPlayer.hpp"
#include "worldMap.hpp"
#include "tilemapRender.hpp"

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

    if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress)) {
      m_localAppStatus = Status::CriticalError;
      logError(1, "Failed to load OpenGL Context: %i %i", Render::MinimumMajorGLVersion, Render::MinimumMinorGLVersion);
      return;
    }

    /* Camera and rendering */
    m_player.add(m_world.set<PosComponent, RotComponent, CameraComponent>());
    m_player.get<PosComponent>() = { 0, 0 };
    m_player.get<RotComponent>() = { 0 };
    m_player.get<CameraComponent>().m_zoom = 100.0f;

    m_render = new Render(m_window, m_player);
    if (m_render->getStatus() == Status::CriticalError) {
      logError(1, "Failed to create render. SDL_GetError(): %s\n", SDL_GetError());
      m_localAppStatus = Status::CriticalError;
      return;
    }

    m_world.addContext<Render*>(m_render);
    m_world.addModule<ShapeRenderModule>();
    m_player.add<RectangleRenderComponent>();
    RectangleRenderComponent& renderRect = m_player.get<RectangleRenderComponent>();
    renderRect.hw = 0.12f;
    renderRect.hh = 0.12f;

    /* Tile drawing ... */
    std::vector<const char*> spritesToLoad = {
      "./res/unknown.png",
      "./res/stone.png",
      "./res/highStone.png",
      "./res/fence.png"
    };

    TilemapRender* tilemapRender = new TilemapRender(m_world, 32, 32, 256);
    m_render->addRenderer(tilemapRender);
    for(int i = 0; i < spritesToLoad.size(); i++)
      if (!tilemapRender->loadSprite(i, spritesToLoad[i])) {
        logError(1, "Failed to load resource: %s.\n", spritesToLoad[i]);
        m_localAppStatus = Status::CriticalError;
        return;
      }

    /* Player Component ... */
    m_world.addModule<PathfindingModule>();
    m_world.addModule<PlayerModule>();
    m_player.add<PlayerComponent>();
    m_player.add<PrimaryTargetTag>();

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

    m_player.get<PosComponent>() = { 5.0f, -7.0f };

    /* WorldMap & Tilemap Component */
    m_world.component<WorldMapTag>();
    m_world.component<TilemapComponent>();
    m_world.component<SpriteComponent>();
    
    { // World Map
      Entity tm = m_world.create();
      logGeneric("World tilemap @ %u\n", tm.id());
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

          if (x <= -35 || x >= 35 || y <= -35 || y >= 35 || (x % 5 == 0 && y < 20 && y != -34)) {
            e.get<SpriteComponent>().texIndex = 2;
            tilemap.insert({ x, y }, bodyId, e, TileFlags::IS_COLLIDABLE);
          }
          else {
            tilemap.insert({ x, y }, bodyId, e, 0);
          }
        };

      callInGrid(-40, -40, 40, 40, tileCallback);
    }

    auto bodyCallback =
      [&](int x, int y) {
      Entity seeker = m_world.create();
      seeker.add<RotComponent>();
      seeker.add<PosComponent>();
      seeker.add<BodyComponent>();
      seeker.add<SeekPrimaryTargetTag>();
      seeker.add<RectangleRenderComponent>();

      RectangleRenderComponent& rectRender = seeker.get<RectangleRenderComponent>();
      rectRender.hw = 0.025f;
      rectRender.hh = 0.025f;

      seeker.get<PosComponent>() = { x * 0.3f - 7, y * 0.3f + 7 };
      seeker.get<RotComponent>() = Rot(0);
      b2BodyDef bodyDef = b2DefaultBodyDef();
      bodyDef.type = b2_dynamicBody;
      bodyDef.position = Vec2(0, 0);
      bodyDef.linearDamping = 10;
      b2ShapeDef shapeDef = b2DefaultShapeDef();
      b2Polygon polygon = b2MakeOffsetBox(0.025, 0.025, Vec2(0), Rot(0));
      b2BodyId bodyId = b2CreateBody(m_physicsWorld, &bodyDef);
      seeker.get<BodyComponent>().id = bodyId;
      b2CreatePolygonShape(bodyId, &shapeDef, &polygon);
      };

    callInGrid(-2, -2, 2, 2, bodyCallback);
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

    if (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_F3]) {
      auto bodyCallback =
        [&](int x, int y) {
        Entity seeker = m_world.create();
        seeker.add<RotComponent>();
        seeker.add<PosComponent>();
        seeker.add<BodyComponent>();
        seeker.add<SeekPrimaryTargetTag>();
        seeker.add<RectangleRenderComponent>();

        RectangleRenderComponent& rectRender = seeker.get<RectangleRenderComponent>();
        rectRender.hw = 0.05f;
        rectRender.hh = 0.05f;

        seeker.get<PosComponent>() = { x * 0.3f - 7, y * 0.3f + 7};
        seeker.get<RotComponent>() = Rot(0);
        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type = b2_dynamicBody;
        bodyDef.position = Vec2(0, 0);
        bodyDef.linearDamping = 10;
        b2ShapeDef shapeDef = b2DefaultShapeDef();
        b2Polygon polygon = b2MakeOffsetBox(0.05, 0.05, Vec2(0), Rot(0));
        b2BodyId bodyId = b2CreateBody(m_physicsWorld, &bodyDef);
        seeker.get<BodyComponent>().id = bodyId;
        b2CreatePolygonShape(bodyId, &shapeDef, &polygon);
        };

      callInGrid(-2, -2, 2, 2, bodyCallback);
    }

    m_world.run(deltaTime);
  }

  void update(float frameTime) {
    static float dt = 0;
    dt += frameTime;

    m_render->mesh();
    m_render->render();
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