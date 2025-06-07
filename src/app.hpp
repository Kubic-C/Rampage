#pragma once

#include "modules/guiRender.hpp"
#include "modules/health.hpp"
#include "modules/item.hpp"
#include "modules/pathfinding.hpp"
#include "modules/physics.hpp"
#include "modules/player.hpp"
#include "modules/shapeRender.hpp"
#include "modules/spriteRender.hpp"
#include "modules/tilemap.hpp"
#include "modules/turret.hpp"

#include "states/menuState.hpp"
#include "states/playState.hpp"

#include "assetLoader.hpp"
#include "eventManager.hpp"

#include "utility/box2dScheduler.hpp"

class App {
  public:
  App(const std::string_view& appName, float ticksPerSecond) :
      m_localAppStatus(Status::Ok), m_ticksPerSecond(ticksPerSecond) {
    /* Really dont wanna forget this ... */
    m_world.addContext<enki::TaskScheduler>();
    enki::TaskScheduler& scheduler = m_world.getContext<enki::TaskScheduler>();
    scheduler.Initialize();

    m_world.addContext<EventManager>();
    m_world.addContext<AppStats>();
    m_world.addContext<DoExit>();

    /* Window and render setup */
    m_world.addContext<SDL_Window*>();
    Render::setNecessaryGLAttributes();
    SDL_Window* window = SDL_CreateWindow(appName.data(), 800, 600, Render::getNecessaryWindowFlags());
    if (!window) {
      m_localAppStatus = Status::CriticalError;
      logError(1, "Failed to create window. SDL_GetError(): %s\n", SDL_GetError());
      return;
    }
    m_world.getContext<SDL_Window*>() = window;
    {
      int x, y, channels;
      u8* data = stbi_load("./res/appIcon.png", &x, &y, &channels, STBI_rgb_alpha);
      if (data != nullptr) {
        SDL_SetWindowIcon(window, SDL_CreateSurfaceFrom(x, y, SDL_PIXELFORMAT_RGBA32, data, x * 4));
      }
    }

    /* Camera and rendering */
    m_world.addContext<Render>(m_world, false);
    Render& render = m_world.getContext<Render>();
    if (render.getStatus() == Status::CriticalError) {
      logError(1, "Failed to create render. SDL_GetError(): %s\n", SDL_GetError());
      m_localAppStatus = Status::CriticalError;
      return;
    }

    /* Gui Setup */
    m_world.addContext<tgui::Gui>(window);
    tgui::Gui& gui = m_world.getContext<tgui::Gui>();
    gui.loadWidgetsFromFile("./res/form.txt");

    m_world.component<WorldMapTag>();

    b2WorldDef physicsWorldDef = b2DefaultWorldDef();
    physicsWorldDef.gravity = b2Vec2{0};
    physicsWorldDef.workerCount = scheduler.GetNumTaskThreads();
    physicsWorldDef.userTaskContext = &scheduler;
    physicsWorldDef.enqueueTask = enki_b2EnqueueTaskCallback;
    physicsWorldDef.finishTask = enki_b2FinishTaskCallback;
    m_world.addContext<b2WorldId>(b2CreateWorld(&physicsWorldDef));

    PhysicsModule::registerComponents(m_world);
    TilemapModule::registerComponents(m_world);
    PathfindingModule::registerComponents(m_world);
    ItemModule::registerComponents(m_world);
    PlayerModule::registerComponents(m_world);
    HealthModule::registerComponents(m_world);
    TurretModule::registerComponents(m_world);
    ShapeRenderModule::registerComponents(m_world);
    SpriteRenderModule::registerComponents(m_world);

    m_world.addModule<PhysicsModule>(4);
    m_world.addModule<TilemapModule>();
    m_world.addModule<PathfindingModule>();
    m_world.addModule<ItemModule>();
    m_world.addModule<PlayerModule>();
    m_world.addModule<HealthModule>();
    m_world.addModule<TurretModule>();
    m_world.addModule<ShapeRenderModule>(0).add<IsRender>();
    m_world.addModule<SpriteRenderModule>(0, 32, 32, 256).add<IsRender>();
    m_world.addModule<GuiRenderModule>(SIZE_MAX, gui).add<IsRender>();

    // Enable the core modules
    m_world.enableModule<TilemapModule>();
    m_world.enableModule<GuiRenderModule>();
    m_world.enableModule<ShapeRenderModule>();
    m_world.enableModule<SpriteRenderModule>();
    m_world.enableModule<ItemModule>();
    m_world.enableModule<HealthModule>();
    m_world.enableModule<TurretModule>();

    /* Asset Loader */
    m_world.addContext<InventoryManager>(m_world);
    m_world.addContext<AssetLoader>(m_world);
    auto& loader = m_world.getContext<AssetLoader>();
    loader.loadAssets("./res/root.json");

    auto& itemMgr = m_world.getContext<InventoryManager>();
    itemMgr.setDefaultItemIcon("./res/clear.png");

    /* State Management, init starts with menuState */
    m_world.addContext<StateManager>();
    auto& stateMgr = m_world.getContext<StateManager>();
    stateMgr.createState<PlayState>("PlayState", m_world);
    stateMgr.createState<MenuState>("MenuState", m_world);
    stateMgr.enableState("MenuState");
  }

  Status getStatus() { return m_localAppStatus; }

  float now() {
    auto current_time = std::chrono::steady_clock::now();

    return (float)std::chrono::duration_cast<std::chrono::microseconds>(current_time - start_time).count() /
        (1000000);
  }

  void tick(u32 tick, float deltaTime) {
    auto& eventMgr = m_world.getContext<EventManager>();
    auto& gui = m_world.getContext<tgui::Gui>();
    auto& stateMgr = m_world.getContext<StateManager>();
    auto& render = m_world.getContext<Render>();
    auto& doExit = m_world.getContext<DoExit>();

    stateMgr.onTick(tick, deltaTime);
    m_world.run(deltaTime);

    eventMgr.poll();
    doExit.exit = eventMgr.shouldExit();
    if (eventMgr.hasWindowReized())
      render.resizeViewportToScreenDim();
    for (SDL_Event event : eventMgr.getPolledEvents()) {
      gui.handleEvent(event);
    }
  }

  void update(float frameTime) {
    enki::TaskScheduler& scheduler = m_world.getContext<enki::TaskScheduler>();
    Render& render = m_world.getContext<Render>();
    StateManager& stateMgr = m_world.getContext<StateManager>();
    stateMgr.onUpdate();

    render.mesh();
    render.render();
  }

  int run() {
    AppStats& appStats = m_world.getContext<AppStats>();

    const float targetTickTime = 1.0f / m_ticksPerSecond; // e.g., 1/60, or 1/30

    float lastTime = now();
    float tickAccumulator = 0.0f;

    float culmTime = 0.0f;
    u32 culmTicks = 0;
    u32 culmFrames = 0;
    u32 tickI = 0;

    while (!m_world.getContext<DoExit>().exit) {
      float nowTime = now();
      float deltaTime = nowTime - lastTime;
      lastTime = nowTime;

      // Clamp deltaTime to avoid big spikes
      if (deltaTime > 0.25f)
        deltaTime = 0.25f;

      tickAccumulator += deltaTime;

      // Only allow a single tick per frame, even if falling behind
      if (tickAccumulator >= targetTickTime) {
        tick(tickI++, targetTickTime); // *fixed* timestep

        tickAccumulator -= targetTickTime;
        culmTicks++;
      }

      // Frame Update (always)
      update(deltaTime);
      culmFrames++;
      culmTime += deltaTime;

      // Update stats every second
      if (culmTime >= 1.0f) {
        appStats.tps = static_cast<float>(culmTicks);
        appStats.fps = static_cast<float>(culmFrames);
        culmTicks = culmFrames = 0;
        culmTime = 0.0f;
      }
    }

    return 0;
  }

  private:
  Status m_localAppStatus;
  EntityWorld m_world;
  float m_ticksPerSecond;

  std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();
};
