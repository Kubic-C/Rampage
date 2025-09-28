#include "module.hpp"

#include "../render/render.hpp"
#include "../render/camera.hpp"

#include "assetLoader.hpp"
#include "inventory.hpp"

#include "components/components.hpp"
#include "systems/guiRender.hpp"
#include "systems/health.hpp"
#include "systems/item.hpp"
#include "systems/pathfinding.hpp"
#include "systems/physics.hpp"
#include "systems/shapeRender.hpp"
#include "systems/spawner.hpp"
#include "systems/spriteRender.hpp"
#include "systems/tilemap.hpp"

RAMPAGE_START

int GameModule::onLoad() {
  auto& world = m_host->getWorld();

  registerGameComponents(world);

  auto& pipeline = m_host->getPipeline();
  pipeline.createGroup<GameGroup>(60)
    .createStage<GameGroup::TickStage>();

  Entity camera = world.create();
  camera.add<TransformComponent>();
  camera.add<CameraComponent>();
  camera.add<CameraInUseTag>();
  Entity textureMap = world.create();
  textureMap.add<TextureMap3DComponent>();
  textureMap.add<TextureMapInUseTag>();
  loadSpriteRender(*m_host);

  world.addContext<tgui::Gui>(world.getContext<SDL_Window*>());
  auto& gui = world.getContext<tgui::Gui>();
  gui.loadWidgetsFromFile("./res/form.txt");
  loadGuiRender(*m_host);

  world.addContext<InventoryManager>(world);
  auto& invMgr = world.getContext<InventoryManager>();
  invMgr.setDefaultItemIcon("./res/clear.png");

  world.addContext<AssetLoader>(world);
  auto& loader = world.getContext<AssetLoader>();
  loader.loadAssets("./res/root.json");

  {
    TextureMap3DComponent& texMap = *textureMap.get<TextureMap3DComponent>();

    Entity entity = world.create();
    entity.add<TransformComponent>();
    entity.add<SpriteComponent>();
    entity.add<SpriteIndependentTag>();

    auto sprite = entity.get<SpriteComponent>();
    SpriteComponent::SubSprite& subSprite = sprite->subSprites.emplace_back().emplace_back();
    subSprite.addLayer(texMap.getSprite("zombie"),  Vec2(0, 0), 0, WorldLayer::Top);
  }

  loadTilemapSystems(*m_host);
  loadSpawnerSystems(*m_host);
  loadPhysicsSystems(*m_host, 4);
  loadPathfindingSystems(*m_host);
  loadItemSystems(*m_host);
  loadHealthSystems(*m_host);
  loadShapeRender(*m_host);

  return 0;
}

int GameModule::onUnload() {
  return 0;
}

int GameModule::onUpdate() {
  auto& world = m_host->getWorld();

  return 0;
}

RAMPAGE_END