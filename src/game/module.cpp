#include "module.hpp"

#include "../render/camera.hpp"
#include "../render/render.hpp"

#include "components/components.hpp"
#include "systems/gui.hpp"
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

int GameModule::onLoad() {
  auto world = m_host->getWorld();

  registerGameComponents(world);

  EntityPtr textureMap = world->create();
  textureMap.add<TextureMap3DComponent>();
  textureMap.add<TextureMapInUseTag>();
  auto texMap = textureMap.get<TextureMap3DComponent>();
  std::filesystem::recursive_directory_iterator dirIt("./");
  for (const auto& entry : dirIt) {
    if (!entry.is_regular_file())
      continue;
    
    std::string path = entry.path().string();
    if (texMap->isValidImageFile(path)) {
      bool loaded = texMap->loadSprite(path);
      if(!loaded) {
        m_host->log("<bgRed>Failed to load sprite: %s<reset>\n", path.c_str());
      } else {
        m_host->log("Loaded sprite: %s with name <fgYellow>%s<reset>\n", path.c_str(), getFilename(path).c_str());
      }
    }
  }
  loadSpriteRender(*m_host);

  world->addContext<tgui::Gui>(world->getContext<SDL_Window*>());
  auto& gui = world->getContext<tgui::Gui>();
  gui.loadWidgetsFromFile("./res/form.txt");
  loadGuiSystems(*m_host);

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
