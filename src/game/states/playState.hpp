#pragma once

#include "../../common/ecs/taggedWorld.hpp"
#include "../../event/module.hpp"
#include "../../render/module.hpp"
#include "../components/components.hpp"
#include "../systems/inventory.hpp"
#include "state.hpp"

RAMPAGE_START

struct PlayStateTag : SerializableTag, JsonableTag {};

class PlayState : public State {
  const std::string menuName = "PlayMenu";
  const std::string returnBtnName = "PlayReturn";
  const std::string playSaveStateTextName = "PlaySaveState";
  const std::string playLoadStateTextName = "PlayLoadState";
  const std::string tickTextName = "PlayTick";
  const std::string tpsTextName = "PlayTPS";
  const std::string fpsTextName = "PlayFPS";
  const std::string activeBodiesTextName = "PlayActiveBodies";
  const std::string playEntityCountTextName = "PlayEntityCount";

public:
  static void observePlayerDeath(EntityPtr player) {
    IWorldPtr world = player.world();

    if (player.has<HealthComponent>() && player.get<HealthComponent>()->health <= 0) {
      auto& stateMgr = world->getContext<StateManager>();
      stateMgr.disableState("PlayState");
      stateMgr.enableState("LostState");
    }
  }

  explicit PlayState(IWorldPtr _world) {
    _world->registerComponent<PlayStateTag>();
    m_world = TaggedEntityWorld::create(_world, _world->component<PlayStateTag>());

    auto& gui = m_world->getContext<tgui::Gui>();
    m_menu = gui.get(menuName);

    tgui::Button::Ptr returnBtn = gui.get(returnBtnName)->cast<tgui::Button>();
    returnBtn->onMousePress([=]() {
      auto& stateMgr = m_world->getContext<StateManager>();
      stateMgr.disableState("PlayState");
      stateMgr.enableState("MenuState");
    });

    tgui::Button::Ptr saveStateBtn = gui.get(playSaveStateTextName)->cast<tgui::Button>();
    saveStateBtn->onMousePress([=]() {
      auto& serializer = m_world->getSerializer();

      serializer.begin(m_world);
      serializer.queueAllWith(m_world->set<PlayStateTag>());
      serializer.end("saveFile.rampage");
    });

    tgui::Button::Ptr loadStateBtn = gui.get(playLoadStateTextName)->cast<tgui::Button>();
    loadStateBtn->onMousePress([=]() {
      auto& deserializer = m_world->getDeserializer();

      // Testing serialization and deserialization stability.
      m_world->destroyAllEntitiesWith(m_world->set<PlayStateTag, IWorld::Enabled>());
      deserializer.deserializeFromFile(*m_world, "saveFile.rampage");
    });

    m_tickText = gui.get(tickTextName)->cast<tgui::Label>();
    m_tpsText = gui.get(tpsTextName)->cast<tgui::Label>();
    m_fpsText = gui.get(fpsTextName)->cast<tgui::Label>();
    m_activeBodiesText = gui.get(activeBodiesTextName)->cast<tgui::Label>();
    m_entityCountText = gui.get(playEntityCountTextName)->cast<tgui::Label>();

    m_world->observe<ComponentRemovedEvent>(m_world->component<PlayerComponent>(), {}, observePlayerDeath);
  }

  void onEntry() {
    const b2WorldId physicsWorld = m_world->getContext<b2WorldId>();
    auto& stateMgr = m_world->getContext<StateManager>();
    auto& invMgr = m_world->getContext<InventoryManager>();
    auto assetLoader = m_world->getAssetLoader();

    m_world->getHost().setGameWorld(m_world);

    m_menu->setEnabled(true);
    m_menu->setVisible(true);

    /* Player */
    EntityPtr player = assetLoader.cloneAsset("BasicPlayer");
    b2Body_EnableContactEvents(*player.get<BodyComponent>(), true);
    player.add<CameraInUseTag>();

    for (int i = 0; i < 1; i++) {
      invMgr.dropItem(m_world, assetLoader.getAsset("BasicTurretItem"), Vec2(0, 0), 10);
      invMgr.dropItem(m_world, assetLoader.getAsset("BigGunTurretItem"), Vec2(1, 0), 10);
      invMgr.dropItem(m_world, assetLoader.getAsset("FenceItem"), Vec2(2, 0), 10);
      invMgr.dropItem(m_world, assetLoader.getAsset("PlaceableHighStoneItem"), Vec2(3, 1), 10);
      invMgr.dropItem(m_world, assetLoader.getAsset("BasicZombieSpawnerItem"), Vec2(4, 2), 512);
      invMgr.dropItem(m_world, assetLoader.getAsset("FastZombieSpawnableItem"), Vec2(4, 2), 512);
      invMgr.dropItem(m_world, assetLoader.getAsset("BigZombieSpawnableItem"), Vec2(4, 2), 512);
      invMgr.dropItem(m_world, assetLoader.getAsset("WoodItem"), Vec2(2, 0), 10);
      invMgr.dropItem(m_world, assetLoader.getAsset("ChestItem"), Vec2(4, -2), 10);
      invMgr.dropItem(m_world, assetLoader.getAsset("PortItem"), Vec2(5, -2), 64);
      invMgr.dropItem(m_world, assetLoader.getAsset("Conveyor2WayItem"), Vec2(5, -2), 64);
      invMgr.dropItem(m_world, assetLoader.getAsset("Conveyor2WayCornerItem"), Vec2(6, -2), 64);
      invMgr.dropItem(m_world, assetLoader.getAsset("Conveyor3WayItem"), Vec2(7, -2), 64);
      invMgr.dropItem(m_world, assetLoader.getAsset("Conveyor4WayItem"), Vec2(8, -2), 64);
      invMgr.dropItem(m_world, assetLoader.getAsset("SandFloorItem"), Vec2(9, -2), 64);
      invMgr.dropItem(m_world, assetLoader.getAsset("WaterFloorItem"), Vec2(9, -2), 64);
      invMgr.dropItem(m_world, assetLoader.getAsset("GrassFloorItem"), Vec2(9, -2), 64);
    }

    /* WorldMap & Tilemap Component */
    {
      // World Map
      // TilemapManager& tmMgr = m_world->getContext<TilemapManager>();
      // EntityPtr tm = m_world->create();
      // tm.add<TransformComponent>();
      // tm.add<BodyComponent>();
      // tm.add<TilemapComponent>();
      // tm.add<BodyComponent>();
      // tm.add<WorldMapTag>();
      // tm.add<VectorTilemapPathfinding>();
      // auto bodyComp = tm.get<BodyComponent>();
      // b2BodyDef bodyDef = b2DefaultBodyDef();
      // bodyDef.type = b2_dynamicBody;
      // bodyComp->id = b2CreateBody(physicsWorld, &bodyDef);

      // int length = 5;
      // for(int x = -length; x < length * 2; x++)
      //   for(int y = -length; y < length; y++)
      //     tmMgr.insertTile(m_world, tm.id(), glm::ivec2(x, y), TileDirection::Right, assetLoader.getAsset("GrassFloorTile"));

      // // Stone outline around the tilemap
      // for(int x = -length - 1; x <= length * 2; x++) {
      //   tmMgr.insertTile(m_world, tm.id(), glm::ivec2(x, -length - 1), TileDirection::Right, assetLoader.getAsset("PermaHighStoneTile")); 
      //   tmMgr.insertTile(m_world, tm.id(), glm::ivec2(x,  length), TileDirection::Right, assetLoader.getAsset("PermaHighStoneTile"));
      // }
      // for(int y = -length; y < length; y++) {
      //   tmMgr.insertTile(m_world, tm.id(), glm::ivec2(-length - 1, y), TileDirection::Right, assetLoader.getAsset("PermaHighStoneTile")); 
      //   tmMgr.insertTile(m_world, tm.id(), glm::ivec2(length * 2, y), TileDirection::Right, assetLoader.getAsset("PermaHighStoneTile"));
      // }

      EntityPtr chunkLoader = m_world->create();
      chunkLoader.add<ChunkedTilemapComponent>();
      chunkLoader.add<TilemapComponent>();
      chunkLoader.add<VectorTilemapPathfinding>();
      chunkLoader.add<TransformComponent>();
      chunkLoader.add<BodyComponent>();

      auto bodyComp = chunkLoader.get<BodyComponent>();
      b2BodyDef bodyDef = b2DefaultBodyDef();
      bodyDef.type = b2_staticBody;
      bodyComp->id = b2CreateBody(physicsWorld, &bodyDef);

      auto chunkedTilemap = chunkLoader.get<ChunkedTilemapComponent>();
      chunkedTilemap->generator = [](IWorldPtr world, const GeneratorChunkInfo& info) {
        auto assetLoader = world->getAssetLoader();
        TilemapManager& tmMgr = world->getContext<TilemapManager>();
        auto tm = world->getEntity(info.tilemapEntity);

        for (int x = info.minTilePos.x; x < info.maxTilePos.x; ++x) {
          for (int y = info.minTilePos.y; y < info.maxTilePos.y; ++y) {

            glm::ivec2 tilePos(x, y);
            float n = noise2d<int>(x, y, info.seed);

            std::string tile;
            if(n > 0.5f)
              tile = "PermaHighStoneTile";
            else if (n > 0.25f)
              tile = "GrassFloorTile";
            else if (n > -0.08f)      // ← this is the beach zone
              tile = "SandFloorTile";
            else
              tile = "WaterFloorTile";

            tmMgr.insertTile(world, tm, tilePos, TileDirection::Right, assetLoader.getAsset(tile));
          }
        }
      };
    }
  }

  static float isAllowed(b2ShapeId shapeId, b2Vec2 point, b2Vec2 normal, float fraction, void* context) {
    std::vector<b2ShapeId>& shapes = *(std::vector<b2ShapeId>*)context;
    shapes.push_back(shapeId);
    return 0;
  }

  void onTick(u32 tick, float deltaTime) override {
    auto& appStats = m_world->getContext<AppStats>();
    auto& physicsWorldId = m_world->getContext<b2WorldId>();
    auto& tmMgr = m_world->getContext<TilemapManager>();
    auto& evtMod = m_world->getContext<EventModule>();
    auto assetLoader = m_world->getAssetLoader();
    const u32 activeBodies = b2World_GetAwakeBodyCount(physicsWorldId);

    m_tickText->setText("Tick: " + std::to_string(tick));
    m_tpsText->setText("TPS: " + std::to_string(appStats.tps));
    m_fpsText->setText("FPS: " + std::to_string(appStats.fps));
    m_activeBodiesText->setText("Active Rigid Bodies: " + std::to_string(activeBodies));
    m_entityCountText->setText("Entity Count: " + std::to_string(m_world->getEntityCount()));

    Vec2 origin = m_world->getContext<RenderModule>().getWorldCoords(
        m_world->getContext<EventModule>().getMouseCoords());

    std::vector<b2ShapeId> shapes;

    if (evtMod.isKeyPressed(Key::LeftControl)) {
      TileRef tileRef = getTileAtPos(m_world, evtMod.getMouseWorldPos());
      if (tileRef.entity != NullEntityId) {
        tmMgr.removeTile(m_world, tileRef.tilemap, tileRef.layer, tileRef.worldPos);
      }
    }

    auto it = m_world->getWith(m_world->set<TilemapComponent>());
    while (it->hasNext()) {
     EntityPtr tmEntity = it->next();
     tmMgr.checkAndHandleBreakage(m_world, tmEntity);
    }

    if (evtMod.isKeyPressed(Key::P)) {
      TileRef tileRef = getTileAtPos(m_world, evtMod.getMouseWorldPos());
      if (tileRef.entity != NullEntityId) {
        EntityPtr tilemap = m_world->getEntity(tileRef.tilemap);
        Tile& tile = tilemap.get<TilemapComponent>()->getTile(tileRef.layer, tileRef.worldPos);
        tile.rotation = getRotateTileDirection(tile.rotation, 1);
      }
    }

    if (evtMod.isKeyPressed(Key::O)) {
      auto it = m_world->getWith(m_world->set<PlayerComponent>());
      while (it->hasNext()) {
        EntityPtr player = it->next();
        auto invView = player.get<InventoryViewComponent>();
        invView->isVisible = true;
      }

      for (int i = 0; i < 1000; i++) {
        EntityPtr e = m_world->create();
        e.add<TransformComponent>();
        e.add<SpriteComponent>();
        e.disable();
      }
    }

    if (evtMod.isMouseButtonPressed(MouseButton::Right)) {
      TileRef ref = getTileWithComponentAtPos<InventoryViewComponent>(m_world, evtMod.getMouseWorldPos());
      if (ref.entity != NullEntityId) {
        auto tilemapEntity = m_world->getEntity(ref.tilemap);
        auto tilemap = tilemapEntity.get<TilemapComponent>();
        Tile& tile = tilemap->getTile(ref.layer, ref.worldPos);
        auto tileEntity = m_world->getEntity(tile.tileId);
        if(tileEntity.has<InventoryViewComponent>()) {
          auto invViewComp = tileEntity.get<InventoryViewComponent>();
          invViewComp->isVisible = true;
        }
      }
    }
  }

  void onLeave() {
    m_world->getHost().setGameWorld(m_world->getTopWorld());

    m_world->destroyAllEntitiesWith(m_world->set<PlayStateTag>());

    m_menu->setEnabled(false);
    m_menu->setVisible(false);
  }

private:
  tgui::Label::Ptr m_tickText;
  tgui::Label::Ptr m_tpsText;
  tgui::Label::Ptr m_fpsText;
  tgui::Label::Ptr m_activeBodiesText;
  tgui::Label::Ptr m_entityCountText;
  tgui::Widget::Ptr m_menu;
  ComponentSet m_addedPlayerComponents;
  IWorldPtr m_world;
};

RAMPAGE_END
