#pragma once

#include "../../event/module.hpp"
#include "../../render/module.hpp"
#include "../components/components.hpp"
#include "state.hpp"
#include "../../common/ecs/taggedWorld.hpp"
#include "../systems/inventory.hpp"

RAMPAGE_START

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
  explicit PlayState(IWorldPtr _world) {  
    _world->component<OwnedBy<PlayState>>(false);
    m_world = TaggedEntityWorld::create(_world, _world->component<OwnedBy<PlayState>>());

    auto& gui = m_world->getContext<tgui::Gui>();
    m_menu = m_world->getContext<tgui::Gui>().get(menuName);

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
        serializer.queueAllWith(m_world->set<OwnedBy<PlayState>>());
        serializer.end("saveFile.rampage");
    });

    tgui::Button::Ptr loadStateBtn = gui.get(playLoadStateTextName)->cast<tgui::Button>();
    loadStateBtn->onMousePress([=]() {
        auto& deserializer = m_world->getDeserializer();
        auto& serializer = m_world->getSerializer();

        // Testing serialization and deserialization stability.
        for(size_t i = 0; i < 5; i++) {
          serializer.begin(m_world);
          m_world->destroyAllEntitiesWith(m_world->set<OwnedBy<PlayState>, IWorld::Enabled>());
          deserializer.deserializeFromFile(*m_world, "saveFile.rampage");
          serializer.queueAllWith(m_world->set<OwnedBy<PlayState>>());
          serializer.end("saveFile.rampage");
        }
    });

    m_tickText = gui.get(tickTextName)->cast<tgui::Label>();
    m_tpsText = gui.get(tpsTextName)->cast<tgui::Label>();
    m_fpsText = gui.get(fpsTextName)->cast<tgui::Label>();
    m_activeBodiesText = gui.get(activeBodiesTextName)->cast<tgui::Label>();
    m_entityCountText = gui.get(playEntityCountTextName)->cast<tgui::Label>();

    EntityPtr base = m_world->create();
    base.disable();
  }

  void onEntry() {
    const b2WorldId physicsWorld = m_world->getContext<b2WorldId>();
    auto& stateMgr = m_world->getContext<StateManager>();
    auto& invMgr = m_world->getContext<InventoryManager>();
    auto assetLoader = m_world->getAssetLoader();

    m_menu->setEnabled(true);
    m_menu->setVisible(true);

    /* Player */
    EntityPtr player = assetLoader.cloneAsset("BasicPlayer");
    player.add<CameraInUseTag>();
    invMgr.addItem(m_world, player, assetLoader.getAsset("BasicTurretItem"), 64);
    invMgr.addItem(m_world, player, assetLoader.getAsset("BigGunTurretItem"), 4);
    invMgr.addItem(m_world, player, assetLoader.getAsset("WoodItem"), 43);
    invMgr.addItem(m_world, player, assetLoader.getAsset("FenceItem"), 27);
    invMgr.addItem(m_world, player, assetLoader.getAsset("PlaceableHighStoneItem"));
    invMgr.addItem(m_world, player, assetLoader.getAsset("ZombieSpawnableItem"), 21);

    for(int i = 0; i < 20; i++) {
      EntityPtr ptr = assetLoader.cloneAsset("BasicZombie");
      ptr.get<TransformComponent>()->pos = Vec2((i % 100) * 0.5f, (i % 15) * 0.5f);
    }

    /* WorldMap & Tilemap Component */
    {
      // World Map
      TilemapManager& tmMgr = m_world->getContext<TilemapManager>();
      EntityPtr tm = m_world->create();
      tm.add<TransformComponent>();
      tm.add<BodyComponent>();
      tm.add<TilemapComponent>();
      tm.add<BodyComponent>();
      tm.add<WorldMapTag>();
      auto bodyComp = tm.get<BodyComponent>();
      b2BodyDef bodyDef = b2DefaultBodyDef();
      bodyDef.type = b2_staticBody;
      bodyComp->id = b2CreateBody(physicsWorld, &bodyDef);

      int length = 10;
      for(int x = -length; x < length * 10; x++)
        for(int y = -length; y < length; y++)
          tmMgr.insertTile(m_world, tm.id(), glm::ivec3(x, y, 0), assetLoader.cloneAsset("StoneFloorTile"));

      // Stone outline around the tilemap
      for(int x = -length - 1; x <= length * 10; x++) {
        tmMgr.insertTile(m_world, tm.id(), glm::ivec3(x, length, 0), assetLoader.cloneAsset("PermaHighStoneTile"));
        tmMgr.insertTile(m_world, tm.id(), glm::ivec3(x, -length - 1, 0), assetLoader.cloneAsset("PermaHighStoneTile"));
      }
      for(int y = -length; y < length; y++) {
        tmMgr.insertTile(m_world, tm.id(), glm::ivec3(-length - 1, y, 0), assetLoader.cloneAsset("PermaHighStoneTile"));
        tmMgr.insertTile(m_world, tm.id(), glm::ivec3(length * 10, y, 0), assetLoader.cloneAsset("PermaHighStoneTile"));
      }

      m_tm = tm;  
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
    const u32 activeBodies = b2World_GetAwakeBodyCount(physicsWorldId);

    m_tickText->setText("Tick: " + std::to_string(tick));
    m_tpsText->setText("TPS: " + std::to_string(appStats.tps));
    m_fpsText->setText("FPS: " + std::to_string(appStats.fps));
    m_activeBodiesText->setText("Active Rigid Bodies: " + std::to_string(activeBodies));
    m_entityCountText->setText("Entity Count: " + std::to_string(m_world->getEntityCount()));

    Vec2 origin = m_world->getContext<RenderModule>().getWorldCoords(m_world->getContext<EventModule>().getMouseCoords());

    std::vector<b2ShapeId> shapes;

    b2QueryFilter filter;
    filter.maskBits = All;
    filter.categoryBits = All;
    b2World_CastRay(physicsWorldId, origin, Vec2(0), filter, isAllowed, &shapes);
    for(b2ShapeId shape : shapes) {
      EntityPtr entity = b2DataToEntity(m_world, b2Shape_GetUserData(shape));
      if(entity.isNull() || !entity.alive())
        continue;

      if(entity.has<TileComponent>()) {
        auto tileComp = entity.get<TileComponent>();
        // m_world->destroy(tmMgr.removeTile(m_world, tileComp->parent, tileComp->pos));
      }
    }

    tmMgr.checkAndHandleBreakage(m_world, m_tm);
  }

  void onLeave() {
    m_world->destroyAllEntitiesWith(m_world->set<OwnedBy<PlayState>, IWorld::Enabled>());

    m_menu->setEnabled(false);
    m_menu->setVisible(false);
  }

private:
  EntityId m_tm;
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
