#pragma once

#include "tilemap.hpp"
#include "player.hpp"
#include "physics.hpp"
#include "seekPlayer.hpp"
#include "worldMap.hpp"

#include "render/shapes.hpp"
#include "tilemapRender.hpp"
#include "guiRender.hpp"

#include "states/menuState.hpp"
#include "states/playState.hpp"

class App {
public:
  App(const std::string_view& appName, float ticksPerSecond)
    : m_localAppStatus(Status::Ok), m_ticksPerSecond(ticksPerSecond) {
    /* Really dont wanna forget this ... */
    m_world.addContext<AppStats>();
    m_world.addContext<DoExit>();

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
    Entity camera = m_world.create();
    camera.add(m_world.set<PosComponent, RotComponent, CameraComponent>());
    camera.get<PosComponent>() = { 0, 0 };
    camera.get<RotComponent>() = { 0 };
    camera.get<CameraComponent>().m_zoom = 100.0f;

    m_render = new Render(m_window, camera);
    m_world.addContext<Render*>(m_render);
    if (m_render->getStatus() == Status::CriticalError) {
      logError(1, "Failed to create render. SDL_GetError(): %s\n", SDL_GetError());
      m_localAppStatus = Status::CriticalError;
      return;
    }

    m_world.addModule<ShapeRenderModule>(0).add<IsRender>();

    /* Gui Setup */
    m_world.addContext<tgui::Gui>(m_window);
    tgui::Gui& gui = m_world.getContext<tgui::Gui>();
    m_world.addModule<GuiRenderModule>(SIZE_MAX, gui).add<IsRender>();
    gui.loadWidgetsFromFile("./res/form.txt");

    /* Tile drawing ... */
    std::vector<const char*> spritesToLoad = {
      "./res/unknown.png",
      "./res/stone.png",
      "./res/highStone.png",
      "./res/fence.png"
    };

    m_world.addModule<TilemapRenderModule>(0, 32, 32, 256).add<IsRender>();
    TilemapRenderModule& tilemapRender = m_world.getModule<TilemapRenderModule>();
    for(int i = 0; i < spritesToLoad.size(); i++)
      if (!tilemapRender.loadSprite(i, spritesToLoad[i])) {
        logError(1, "Failed to load resource: %s.\n", spritesToLoad[i]);
        m_localAppStatus = Status::CriticalError;
        return;
      }

    /* Player Component ... */
    m_world.addModule<PathfindingModule>();
    m_world.addModule<PlayerModule>();

    m_world.component<WorldMapTag>();
    m_world.component<TilemapComponent>();
    m_world.component<SpriteComponent>();

    m_world.enableModule<GuiRenderModule>();
    m_world.enableModule<ShapeRenderModule>();
    m_world.enableModule<TilemapRenderModule>();

    /* State Management, init starts with menuState */
    m_world.addContext<StateManager>();
    StateManager& stateMgr = m_world.getContext<StateManager>();
    stateMgr.createState<PlayState>("PlayState", m_world);
    stateMgr.createState<MenuState>("MenuState", m_world);
    stateMgr.enableState("MenuState");
  }

  ~App() {
    // tgui does not like the backend be destroyed first (Opengl/Render)
    m_world.destroyContext<StateManager>();
    m_world.destroyContext<tgui::Gui>();
  }

  Status getStatus() {
    return m_localAppStatus;
  }

  float now() {
    auto current_time = std::chrono::high_resolution_clock::now();

    return (float)std::chrono::duration_cast<std::chrono::microseconds>(current_time - start_time).count() / (1000000);
  }

  void tick(u32 tick, float deltaTime) {
    StateManager& stateMgr = m_world.getContext<StateManager>();
    stateMgr.onTick(tick, deltaTime);

    tgui::Gui& gui = m_world.getContext<tgui::Gui>();
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      gui.handleEvent(event);
      switch (event.type) {
      case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
      case SDL_EVENT_QUIT:
        m_world.getContext<DoExit>().exit = true;
        break;
      case SDL_EVENT_WINDOW_RESIZED:
        m_render->resizeViewportToScreenDim();
        break;
      }
    }

    m_world.run(deltaTime);
  }

  void update(float frameTime) {
    StateManager& stateMgr = m_world.getContext<StateManager>();
    stateMgr.onUpdate();

    m_render->mesh();
    m_render->render();
  }

  int run() {
    AppStats& appStats = m_world.getContext<AppStats>();

    float lastTime = 0;
    float ticksToGo = 0;

    float tickLastTime = 0;
    u32 tickI = 0;

    float culmTime = 0;
    u32 culmTicks = 0;
    u32 culmFrames = 0;
   
    while (!m_world.getContext<DoExit>().exit) {
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
        culmTicks++;
      }

      update(deltaTime);
      culmFrames++;
      culmTime += deltaTime;
      if (culmTime >= 1.0f) {
        appStats.tps = (float)culmTicks;
        appStats.fps = (float)culmFrames;
        culmTicks = culmFrames = culmTime = 0;
      }
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

  float m_ticksPerSecond;

  std::chrono::steady_clock::time_point start_time = std::chrono::high_resolution_clock::now();
};