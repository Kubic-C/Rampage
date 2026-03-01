#include "module.hpp"

#include "../render/camera.hpp"
#include "../render/render.hpp"

#include "inventory.hpp"

#include "components/components.hpp"
#include "systems/gui.hpp"
#include "systems/health.hpp"
#include "systems/item.hpp"
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
#include "systems/player.hpp"

RAMPAGE_START

int tickState(IWorldPtr world, float dt) {
  auto& stats = world->getContext<AppStats>();
  auto& stateMgr = world->getContext<StateManager>();

  stateMgr.onTick(stats.tick, dt);

  return 0;
}

int GameModule::onLoad() {
  auto world = m_host->getWorld();

  registerGameComponents(world);

  EntityPtr camera = world->create();
  camera.add<TransformComponent>();
  camera.add<CameraComponent>();
  camera.add<CameraInUseTag>();
  EntityPtr textureMap = world->create();
  textureMap.add<TextureMap3DComponent>();
  textureMap.add<TextureMapInUseTag>();
  loadSpriteRender(*m_host);

  world->addContext<tgui::Gui>(world->getContext<SDL_Window*>());
  auto& gui = world->getContext<tgui::Gui>();
  gui.loadWidgetsFromFile("./res/form.txt");
  loadGuiSystems(*m_host);

  world->addContext<InventoryManager>(world);
  auto& invMgr = world->getContext<InventoryManager>();
  invMgr.setDefaultItemIcon("./res/clear.png");

  world->getAssetLoader().loadAssetsFromFile("./res/root.json");

  loadTilemapSystems(*m_host);
  loadSpawnerSystems(*m_host);
  loadPhysicsSystems(*m_host, 4);
  loadPathfindingSystems(*m_host);
  loadItemSystems(*m_host);
  loadHealthSystems(*m_host);
  loadPlayerSystems(*m_host);
  loadShapeRender(*m_host);
  loadTurretSystems(*m_host);

  world->addContext<StateManager>();
  auto& stateMgr = world->getContext<StateManager>();
  stateMgr.createState<PlayState>("PlayState", world);
  stateMgr.createState<MenuState>("MenuState", world);
  stateMgr.enableState("MenuState");

  m_host->getPipeline().getGroup<GameGroup>().attachToStage<GameGroup::TickStage>(tickState);

  return 0;
}

int GameModule::onUpdate() {
  auto world = m_host->getWorld();
  auto& stateMgr = world->getContext<StateManager>();

  stateMgr.onUpdate();

  return 0;
}

RAMPAGE_END
