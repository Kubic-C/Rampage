#pragma once

#include "../../event/module.hpp"
#include "../../render/module.hpp"
#include "../components/components.hpp"
#include "state.hpp"

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
  explicit PlayState(EntityWorld& world) : m_world(world) {
    m_world.component<OwnedBy<PlayState>>();
    m_addedPlayerComponents = m_world.set<PlayerComponent, PrimaryTargetTag, BodyComponent,
                                          RectangleRenderComponent, InventoryComponent>();

    auto& gui = m_world.getContext<tgui::Gui>();
    m_menu = world.getContext<tgui::Gui>().get(menuName);

    tgui::Button::Ptr returnBtn = gui.get(returnBtnName)->cast<tgui::Button>();
    returnBtn->onMousePress([&]() {
      auto& stateMgr = world.getContext<StateManager>();
      stateMgr.disableState("PlayState");
      stateMgr.enableState("MenuState");
    });

    tgui::Button::Ptr saveStateBtn = gui.get(playSaveStateTextName)->cast<tgui::Button>();
    saveStateBtn->onMousePress([&]() {
      auto& stateMgr = world.getContext<StateManager>();

      auto serWorld = dynamic_cast<EntityWorldSerializable*>(&world);
      if (serWorld) {
        auto saveAllEntitiesWith = world.set<>();
        serWorld->saveState("./dat/testSave.save", saveAllEntitiesWith);
      }
    });

    tgui::Button::Ptr loadStateBtn = gui.get(playLoadStateTextName)->cast<tgui::Button>();
    loadStateBtn->onMousePress([&]() {
      auto& stateMgr = world.getContext<StateManager>();

      auto serWorld = dynamic_cast<EntityWorldSerializable*>(&world);
      if (serWorld) {
        bool appendEntities = false;
        bool clearPreviousEntities = false;
        serWorld->loadState("./dat/testSave.save", appendEntities, clearPreviousEntities);
      }
    });

    m_tickText = gui.get(tickTextName)->cast<tgui::Label>();
    m_tpsText = gui.get(tpsTextName)->cast<tgui::Label>();
    m_fpsText = gui.get(fpsTextName)->cast<tgui::Label>();
    m_activeBodiesText = gui.get(activeBodiesTextName)->cast<tgui::Label>();
    m_entityCountText = gui.get(playEntityCountTextName)->cast<tgui::Label>();

    Entity base = m_world.create();
    base.disable();
    base.add<OwnedBy<PlayState>>();

    m_bodyCallback = [this, baseId = base.id()](int x, int y) {
      Vec2 mousePos = m_world.getContext<RenderModule>().getWorldCoords(
          m_world.getContext<EventModule>().getMouseCoords());

      auto& loader = m_world.getContext<AssetLoader>();

      std::string enemyType = "BasicZombie";
      if (rand() % 20 < 1)
        enemyType = "BigAssZombie";

      Entity seeker = loader.getPrefab(enemyType).clone();
      m_world.get(baseId).copyInto(seeker);
      seeker.get<TransformComponent>()->pos = mousePos + Vec2(x * 0.3f, y * 0.3f) - Vec2(2 * 0.3f, 2 * 0.3f);
    };
  }

  void onEntry() {
    const b2WorldId physicsWorld = m_world.getContext<b2WorldId>();
    auto& stateMgr = m_world.getContext<StateManager>();
    auto& invMgr = m_world.getContext<InventoryManager>();
    auto& assetLoader = m_world.getContext<AssetLoader>();
    Entity player = m_world.getFirstWith(m_world.set<CameraInUseTag>());

    m_menu->setEnabled(true);
    m_menu->setVisible(true);

    /* Player */
    player.add(m_addedPlayerComponents);
    Inventory playerInvetory = invMgr.createInventory("Player Inventory", 3, 5);
    player.get<InventoryComponent>()->id = playerInvetory;
    playerInvetory.addItem(assetLoader.getPrefab("BasicTurretItem"), 20);
    playerInvetory.addItem(assetLoader.getPrefab("PlaceableHighStoneItem"), 2);
    playerInvetory.addItem(assetLoader.getPrefab("WoodItem"), 32);
    playerInvetory.addItem(assetLoader.getPrefab("BigGunItem"), 4);
    playerInvetory.addItem(assetLoader.getPrefab("BigGunItem"), 2);

    // Render
    auto renderRect = player.get<RectangleRenderComponent>();
    renderRect->hw = 0.12f;
    renderRect->hh = 0.12f;

    // Collider
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.fixedRotation = true;
    bodyDef.linearDamping = 10;
    b2BodyId bodyId = b2CreateBody(physicsWorld, &bodyDef);
    player.get<BodyComponent>()->id = bodyId;
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1000;
    b2Polygon rect = b2MakeBox(0.12f, 0.12f);
    b2CreatePolygonShape(bodyId, &shapeDef, &rect);

    RefT<TransformComponent> playerTransform = player.get<TransformComponent>();
    playerTransform->pos = Vec2(5.0f, -7.0f);

    /* WorldMap & Tilemap Component */

    {
      // World Map
      Entity tm = m_world.create();
      tm.add<TransformComponent>();
      tm.add<BodyComponent>();
      tm.add<TilemapComponent>();
      tm.add<WorldMapTag>();
      tm.add<OwnedBy<PlayState>>();

      RefT<TransformComponent> transform = tm.get<TransformComponent>();
      transform->pos = Vec2(0, 0);
      transform->rot = Rot(0);
      b2BodyDef bodyDef = b2DefaultBodyDef();
      bodyDef.type = b2_staticBody;
      bodyDef.position = Vec2(0, 0);
      b2BodyId bodyId = b2CreateBody(physicsWorld, &bodyDef);
      tm.get<BodyComponent>()->id = bodyId;

      RefT<TilemapComponent> tilemapLayers = tm.get<TilemapComponent>();
      Tilemap& worldLayer = tilemapLayers->getTilemap(TilemapWorldLayer);
      auto tileCallback = [&](int x, int y) {
        if (x <= -35 || x >= 35 || y <= -35 || y >= 35 || (x % 5 == 0 && y < 20 && y != -34)) {
          TileDef highStone = assetLoader.cloneTilePrefab("PermaHighStoneTile");
          worldLayer.insert(m_world, bodyId, {x, y}, tm, highStone);
        } else if (rand() % 100 < 5) {
          TileDef unknown = assetLoader.cloneTilePrefab("UnknownTile");
          worldLayer.insert(m_world, bodyId, {x, y}, tm, unknown);
        } else {
          TileDef stone = assetLoader.cloneTilePrefab("StoneFloorTile");
          worldLayer.insert(m_world, bodyId, {x, y}, tm, stone);
        }
      };

      callInGrid(-37, -37, 37, 37, tileCallback);
    }
  }

  void onTick(u32 tick, float deltaTime) override {
    auto& appStats = m_world.getContext<AppStats>();
    auto& physicsWorldId = m_world.getContext<b2WorldId>();
    const u32 activeBodies = b2World_GetAwakeBodyCount(physicsWorldId);

    m_tickText->setText("Tick: " + std::to_string(tick));
    m_tpsText->setText("TPS: " + std::to_string(appStats.tps));
    m_fpsText->setText("FPS: " + std::to_string(appStats.fps));
    m_activeBodiesText->setText("Active Rigid Bodies: " + std::to_string(activeBodies));
    int count = 0;
    auto eIt = m_world.getWith(
        m_world.set<LifetimeComponent, HealthComponent, ContactDamageComponent, BodyComponent,
                    SubmitToCollisionQueueComponent, CircleRenderComponent, TransformComponent>());
    while (eIt.hasNext()) {
      eIt.next();
      count++;
    }

    m_entityCountText->setText("Set Count: " + std::to_string(m_world.getSetCount()));

    const b2WorldId physicsWorld = m_world.getContext<b2WorldId>();
    if (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_F3] && tick % 3 == 0) {
      callInGrid(-2, -2, 2, 2, m_bodyCallback);
    }
  }

  void onLeave() {
    Entity player = m_world.getFirstWith(m_world.set<CameraComponent>());

    m_world.destroyAllEntitiesWith(m_world.set<OwnedBy<PlayState>, EntityWorld::Enabled>());
    player.remove(m_addedPlayerComponents);

    m_menu->setEnabled(false);
    m_menu->setVisible(false);
  }

private:
  std::function<void(int, int)> m_bodyCallback;
  tgui::Label::Ptr m_tickText;
  tgui::Label::Ptr m_tpsText;
  tgui::Label::Ptr m_fpsText;
  tgui::Label::Ptr m_activeBodiesText;
  tgui::Label::Ptr m_entityCountText;
  tgui::Widget::Ptr m_menu;
  ComponentSet m_addedPlayerComponents;
  EntityWorld& m_world;
};

RAMPAGE_END
