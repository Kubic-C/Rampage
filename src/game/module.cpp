#include "module.hpp"

#include "../render/camera.hpp"

#include "components/components.hpp"
#include "systems/health.hpp"
#include "systems/pathfinding.hpp"
#include "systems/physics.hpp"
#include "systems/shapeRender.hpp"
#include "systems/spawner.hpp"
#include "systems/spriteRender.hpp"
#include "systems/tilemap.hpp"
#include "systems/turret.hpp"

#include "states/menuState.hpp"
#include "states/playState.hpp"
#include "states/state.hpp"
#include "states/lostState.hpp"
#include "systems/player.hpp"
#include "systems/inventory.hpp"

RAMPAGE_START

int tickState(IWorldPtr world, float dt) {
  auto& stats = world->getContext<AppStats>();
  auto& stateMgr = world->getContext<StateManager>();

  stateMgr.onTick(stats.tick, dt);

  return 0;
}

int clearDrawCmds(IWorldPtr world, float dt) {
  auto& render2D = world->getContext<Render2D>();
  render2D.clearCmds();
  return 0;
}

int GameModule::onLoad() {
  auto world = m_host->getWorld();

  registerGameComponents(world);
  loadSpriteRender(*m_host);

  world->addContext<InventoryManager>();
  world->getAssetLoader().loadAssetsFromFile("./res/root.json");
  loadTilemapSystems(*m_host);
  loadSpawnerSystems(*m_host);
  loadPhysicsSystems(*m_host, 4);
  loadPathfindingSystems(*m_host);
  loadHealthSystems(*m_host);
  loadPlayerSystems(*m_host);
  loadShapeRender(*m_host);
  loadTurretSystems(*m_host);
  loadInventorySystems(*m_host);

  world->addContext<StateManager>();
  auto& stateMgr = world->getContext<StateManager>();
  stateMgr.createState<PlayState>("PlayState", world);
  stateMgr.createState<MenuState>("MenuState", world);
  stateMgr.createState<LostState>("LostState", world);
  stateMgr.enableState("PlayState");

  m_host->getPipeline().getGroup<GameGroup>().attachToStage<GameGroup::TickStage>(tickState);
  m_host->getPipeline().getGroup<GameGroup>().attachToStage<GameGroup::PreTickStage>(clearDrawCmds);

  return 0;
}

int GameModule::onUpdate() {
  auto world = m_host->getWorld();
  auto& stateMgr = world->getContext<StateManager>();

  stateMgr.onUpdate();

  return 0;
}

RAMPAGE_END
