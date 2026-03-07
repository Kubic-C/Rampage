#pragma once

#include "../../event/module.hpp"
#include "../../render/module.hpp"
#include "../components/components.hpp"
#include "state.hpp"
#include "../../common/ecs/taggedWorld.hpp"
#include "../systems/inventory.hpp"

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
  explicit PlayState(IWorldPtr _world) {  
    _world->component<PlayStateTag>(false);
    m_world = TaggedEntityWorld::create(_world, _world->component<PlayStateTag>());

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
  }

  void onEntry() {
    const b2WorldId physicsWorld = m_world->getContext<b2WorldId>();
    auto& stateMgr = m_world->getContext<StateManager>();
    auto& invMgr = m_world->getContext<InventoryManager>();
    auto assetLoader = m_world->getAssetLoader();

    m_world->getHost().setGameWorld(m_world);
    std::cout << m_world << std::endl;

    m_menu->setEnabled(true);
    m_menu->setVisible(true);

    /* Player */
    EntityPtr player = assetLoader.cloneAsset("BasicPlayer");
    b2Body_EnableContactEvents(*player.get<BodyComponent>(), true);
    player.add<CameraInUseTag>();
    invMgr.addItem(m_world, player, assetLoader.getAsset("BasicTurretItem"), 64);
    invMgr.addItem(m_world, player, assetLoader.getAsset("BigGunTurretItem"), 4);
    invMgr.addItem(m_world, player, assetLoader.getAsset("WoodItem"), 43);
    invMgr.addItem(m_world, player, assetLoader.getAsset("FenceItem"), 27);
    invMgr.addItem(m_world, player, assetLoader.getAsset("PlaceableHighStoneItem"));
    invMgr.addItem(m_world, player, assetLoader.getAsset("ZombieSpawnableItem"), 21);

    for(int i = 0; i < 1; i++) {
      invMgr.dropItem(m_world, assetLoader.getAsset("BasicTurretItem"), Vec2(0, 0), 10);
      invMgr.dropItem(m_world, assetLoader.getAsset("BigGunTurretItem"), Vec2(1, 0), 10);
      invMgr.dropItem(m_world, assetLoader.getAsset("WoodItem"), Vec2(2, 0), 10);
      invMgr.dropItem(m_world, assetLoader.getAsset("FenceItem"), Vec2(3, 0), 10);
      invMgr.dropItem(m_world, assetLoader.getAsset("PlaceableHighStoneItem"), Vec2(4, 1), 10);
      invMgr.dropItem(m_world, assetLoader.getAsset("ZombieSpawnableItem"), Vec2(4, 2), 10);
    }

    for(int i = 0; i < 0; i++) {
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

      int length = 50;
      for(int x = -length; x < length * 2; x++)
        for(int y = -length; y < length; y++)
          tmMgr.insertTile(m_world, tm.id(), WorldLayer::Floor, glm::ivec2(x, y), assetLoader.cloneAsset("StoneFloorTile"));

      // Stone outline around the tilemap
      for(int x = -length - 1; x <= length * 2; x++) {
        tmMgr.insertTile(m_world, tm.id(), WorldLayer::Floor, glm::ivec2(x, length), assetLoader.cloneAsset("PermaHighStoneTile"));
        tmMgr.insertTile(m_world, tm.id(), WorldLayer::Floor, glm::ivec2(x, -length - 1), assetLoader.cloneAsset("PermaHighStoneTile"));
      }
      for(int y = -length; y < length; y++) {
        tmMgr.insertTile(m_world, tm.id(), WorldLayer::Floor, glm::ivec2(-length - 1, y), assetLoader.cloneAsset("PermaHighStoneTile"));
        tmMgr.insertTile(m_world, tm.id(), WorldLayer::Floor, glm::ivec2(length * 10, y), assetLoader.cloneAsset("PermaHighStoneTile"));
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
    auto& evtMod = m_world->getContext<EventModule>();
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

      if(entity.has<TileComponent>() && evtMod.isKeyHeld(Key::LeftControl)) {
        auto tileComp = entity.get<TileComponent>();
        tmMgr.removeTile(m_world, tileComp->parent, tileComp->layer, tileComp->pos, true);
      }
    }

    auto it = m_world->getWith(m_world->set<TilemapComponent>());
    while(it->hasNext()) {
      EntityPtr tmEntity = it->next();
      tmMgr.checkAndHandleBreakage(m_world, tmEntity);
    }
  }

  void onLeave() {
    m_world->getHost().setGameWorld(m_world->getTopWorld());

    m_world->destroyAllEntitiesWith(m_world->set<PlayStateTag, IWorld::Enabled>());

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
