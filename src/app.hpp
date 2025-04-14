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

    /* Player Components ... */
    m_world.addModule<PathfindingModule>();
    m_world.addModule<PlayerModule>();

    m_world.component<WorldMapTag>();
    m_world.component<TilemapComponent>();
    m_world.component<SpriteComponent>();

    /* Tile Prefabs */
    m_world.addContext<TilePrefabs>(m_world);
    TilePrefabs& tilePrefab = m_world.getContext<TilePrefabs>();
    Entity unknown = m_world.create();
    unknown.add<SpriteComponent>();
    unknown.get<SpriteComponent>().texIndex = 0;
    tilePrefab.createPrefab("Unknown", unknown, 0, { 1, 1 });
    Entity stone = m_world.create();
    stone.add<SpriteComponent>();
    stone.add<ArrowComponent>();
    stone.get<SpriteComponent>().texIndex = 1;
    tilePrefab.createPrefab("StoneFloor", stone, 0, { 1, 1 });
    Entity highStone = m_world.create();
    highStone.add<SpriteComponent>();
    highStone.get<SpriteComponent>().texIndex = 2;
    tilePrefab.createPrefab("HighStone", highStone, TileFlags::IS_COLLIDABLE, { 1, 1 });

    /* GUI renderer */
    m_world.addModule<GuiRenderModule>(SIZE_MAX, gui).add<IsRender>();

    /* Shape Renderer */
    m_world.addModule<ShapeRenderModule>(0).add<IsRender>();

    /* Tilemap Render */
    std::vector<const char*> spritesToLoad = {
      "./res/unknown.png",
      "./res/stone.png",
      "./res/highStone.png",
      "./res/fence.png"
    };
    m_world.addModule<TilemapRenderModule>(0, 32, 32, 256).add<IsRender>();
    TilemapRenderModule& tilemapRender = m_world.getModule<TilemapRenderModule>();
    for (int i = 0; i < spritesToLoad.size(); i++)
      if (!tilemapRender.loadSprite(i, spritesToLoad[i])) {
        logError(1, "Failed to load resource: %s.\n", spritesToLoad[i]);
        m_localAppStatus = Status::CriticalError;
        return;
      }

    // Enable All rendering modules
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
      case SDL_EVENT_WINDOW_RESIZED: {
        Render& render = m_world.getContext<Render>();
        render.resizeViewportToScreenDim();
      } break;
      }
    }

    m_world.run(deltaTime);
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