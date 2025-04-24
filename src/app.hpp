#pragma once

#include "modules/guiRender.hpp"
#include "modules/health.hpp"
#include "modules/pathfinding.hpp"
#include "modules/physics.hpp"
#include "modules/player.hpp"
#include "modules/shapeRender.hpp"
#include "modules/spriteRender.hpp"
#include "modules/tilemap.hpp"
#include "modules/turret.hpp"
#include "modules/item.hpp"

#include "states/menuState.hpp"
#include "states/playState.hpp"

#include "eventManager.hpp"
#include "assetLoader.hpp"

class App {
public:
  App(const std::string_view& appName, float ticksPerSecond)
    : m_localAppStatus(Status::Ok), m_ticksPerSecond(ticksPerSecond) {
    /* Really dont wanna forget this ... */
    m_world.addContext<EventManager>();
    m_world.addContext<AppStats>();
    m_world.addContext<DoExit>();

    /* Physics setup */
    b2WorldDef physicsWorldDef = b2DefaultWorldDef();
    physicsWorldDef.gravity = b2Vec2{ 0 };
    m_world.addContext<b2WorldId>(b2CreateWorld(&physicsWorldDef));
    m_world.addModule<PhysicsModule>(4);

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

    m_world.addModule<TilemapModule>();
    m_world.addModule<PathfindingModule>();
    m_world.addModule<ItemModule>();
    m_world.addModule<PlayerModule>();
    m_world.addModule<HealthModule>();
    m_world.addModule<TurretModule>();

    m_world.component<WorldMapTag>();

    /* GUI renderer */
    m_world.addModule<GuiRenderModule>(SIZE_MAX, gui).add<IsRender>();

    /* Shape Renderer */
    m_world.addModule<ShapeRenderModule>(0).add<IsRender>();

    /* Tilemap Render */
    m_world.addModule<SpriteRenderModule>(0, 32, 32, 256).add<IsRender>();

    /* Asset Loader */
    m_world.addContext<AssetLoader>(m_world);
    AssetLoader& loader = m_world.getContext<AssetLoader>();
    loader.loadAssets("./res/sprites.json"); // ALWAYS LOAD SPRITES FIRST!
    loader.loadAssets("./res/items.json");
    loader.loadAssets("./res/tiles.json");

    // Enable the core modules
    m_world.enableModule<TilemapModule>();
    m_world.enableModule<GuiRenderModule>();
    m_world.enableModule<ShapeRenderModule>();
    m_world.enableModule<SpriteRenderModule>();
    m_world.enableModule<ItemModule>();
    m_world.enableModule<HealthModule>();
    m_world.enableModule<TurretModule>();

    InventoryManager& itemMgr = m_world.getContext<InventoryManager>();
    itemMgr.setDefaultItemIcon("./res/clear.png");

    /* State Management, init starts with menuState */
    m_world.addContext<StateManager>();
    StateManager& stateMgr = m_world.getContext<StateManager>();
    stateMgr.createState<PlayState>("PlayState", m_world);
    stateMgr.createState<MenuState>("MenuState", m_world);
    stateMgr.enableState("MenuState");
  }

  Status getStatus() {
    return m_localAppStatus;
  }

  float now() {
    auto current_time = std::chrono::high_resolution_clock::now();

    return (float)std::chrono::duration_cast<std::chrono::microseconds>(current_time - start_time).count() / (1000000);
  }

  void tick(u32 tick, float deltaTime) {
    EventManager& eventMgr = m_world.getContext<EventManager>();
    tgui::Gui& gui = m_world.getContext<tgui::Gui>();
    StateManager& stateMgr = m_world.getContext<StateManager>();
    Render& render = m_world.getContext<Render>();
    DoExit& doExit = m_world.getContext<DoExit>();

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
    Render& render = m_world.getContext<Render>();
    StateManager& stateMgr = m_world.getContext<StateManager>();
    stateMgr.onUpdate();

    render.mesh();
    render.render();
  }

  int run() {
    AppStats& appStats = m_world.getContext<AppStats>();

    // For Frames
    float lastTime = 0;
    float ticksToGo = 0;

    // For Ticks
    float tickLastTime = 0;
    u32 tickI = 0;

    // For App Stats
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
  EntityWorld m_world;
  float m_ticksPerSecond;

  std::chrono::steady_clock::time_point start_time = std::chrono::high_resolution_clock::now();
};