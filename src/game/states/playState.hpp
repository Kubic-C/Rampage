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

    m_addedPlayerComponents = m_world->set<PlayerComponent, PrimaryTargetTag, BodyComponent,
                                          RectangleRenderComponent>();

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
        EntityPtr player = m_world->getFirstWith(m_world->set<CameraInUseTag>());
        
        serializer.begin(m_world);
        serializer.queueAllWith(m_world->set<OwnedBy<PlayState>>());
        serializer.queue(player, player.set());
        serializer.end("saveFile.rampage");
    });

    tgui::Button::Ptr loadStateBtn = gui.get(playLoadStateTextName)->cast<tgui::Button>();
    loadStateBtn->onMousePress([=]() {
        auto& deserializer = m_world->getDeserializer();
        m_world->destroyAllEntitiesWith(m_world->set<OwnedBy<PlayState>, IWorld::Enabled>());
        deserializer.deserializeFromFile(*m_world, "saveFile.rampage");
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
    EntityPtr player = m_world->getFirstWith(m_world->set<CameraInUseTag>());

    m_menu->setEnabled(true);
    m_menu->setVisible(true);

    /* Player */
    player.add(m_addedPlayerComponents);

    EntityPtr basicTurretItem = m_world->create();
    basicTurretItem.add<ItemComponent>();
    auto itemComp = basicTurretItem.get<ItemComponent>(); 
    itemComp->name = "Basic Turret";
    itemComp->icon = tgui::Texture("res/entities/basicTurretIcon.png");
    itemComp->isUnique = false;
    itemComp->maxStackSize = 16;
    itemComp->stackCost = 4;
 
    player.add<InventoryComponent>();
    invMgr.createInventory(m_world, player, 5, 5);
    player.add<InventoryViewComponent>();
    auto invView = player.get<InventoryViewComponent>();
    invView->inventoryEntityId = player;
    invView->name = "Player Inventory";
    invView->isVisible = true;

    {
      EntityPtr e = m_world->create();
      e.add<InventoryComponent>();
      invMgr.createInventory(m_world, e, 5, 5);
      e.add<InventoryViewComponent>();
      auto invView = e.get<InventoryViewComponent>();
      invView->inventoryEntityId = e;
      invView->name = "Other Inventory";
      invView->isVisible = true;
    }

    invMgr.addItem(m_world, player.id(), basicTurretItem.id(), 10);

    // Render
    auto renderRect = player.get<RectangleRenderComponent>();
    renderRect->hw = 0.12f;
    renderRect->hh = 0.12f;

    // Collider
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.linearDamping = 10;
    b2BodyId bodyId = b2CreateBody(physicsWorld, &bodyDef);
    player.get<BodyComponent>()->id = bodyId;
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1000;
    b2Polygon rect = b2MakeBox(0.12f, 0.12f);
    b2CreatePolygonShape(bodyId, &shapeDef, &rect);

    RefT<TransformComponent> playerTransform = player.get<TransformComponent>();
    playerTransform->pos = Vec2(0.0f, 8.0f);

    assetLoader.cloneAsset("BigAssZombie");

    /* WorldMap & Tilemap Component */
    {
      // World Map
      TilemapManager& tmMgr = m_world->getContext<TilemapManager>();
      EntityPtr tm = m_world->create();
      tm.add<TransformComponent>();
      tm.add<BodyComponent>();
      tm.add<TilemapComponent>();
      tm.add<WorldMapTag>();
      tm.add<BodyComponent>();
      auto bodyComp = tm.get<BodyComponent>();
      b2BodyDef bodyDef = b2DefaultBodyDef();
      bodyDef.type = b2_staticBody;
      bodyComp->id = b2CreateBody(physicsWorld, &bodyDef);

      int length = 52;
      int radius = length / 2;
      int center = length / 2;
      for(int x = -center; x < center; x++) {
        for(int y = -center; y < center; y++) {
          if(glm::distance(glm::vec2(x, y), glm::vec2(0, 0)) <= radius) {
            tmMgr.insertTile(m_world, tm.id(), glm::ivec3(x, y, 0), assetLoader.cloneAsset("StoneFloorTile"));
          } else {
            tmMgr.insertTile(m_world, tm.id(), glm::ivec3(x, y, 0), assetLoader.cloneAsset("PermaHighStoneTile"));
          }
        }
      }

      m_tm = tm;  
    }
  }

  void onTick(u32 tick, float deltaTime) override {
    auto& appStats = m_world->getContext<AppStats>();
    auto& physicsWorldId = m_world->getContext<b2WorldId>();
    const u32 activeBodies = b2World_GetAwakeBodyCount(physicsWorldId);

    m_tickText->setText("Tick: " + std::to_string(tick));
    m_tpsText->setText("TPS: " + std::to_string(appStats.tps));
    m_fpsText->setText("FPS: " + std::to_string(appStats.fps));
    m_activeBodiesText->setText("Active Rigid Bodies: " + std::to_string(activeBodies));
    int count = 0;
    auto eIt = m_world->getWith(
        m_world->set<LifetimeComponent, HealthComponent, ContactDamageComponent, BodyComponent, CircleRenderComponent, TransformComponent>());
    while (eIt->hasNext()) {
      eIt->next();
      count++;
    }

    m_entityCountText->setText("Set Count: " + std::to_string(m_world->getSetCount()));
  }

  void onLeave() {
    EntityPtr player = m_world->getFirstWith(m_world->set<CameraComponent>());

    m_world->destroyAllEntitiesWith(m_world->set<OwnedBy<PlayState>, IWorld::Enabled>());
    player.remove(m_addedPlayerComponents);

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
