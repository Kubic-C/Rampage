#pragma once

#include "../components/arrow.hpp"
#include "../components/body.hpp"
#include "../components/player.hpp"
#include "../components/shapes.hpp"
#include "state.hpp"

class PlayState : public State {
  const std::string& menuName = "PlayMenu";
  const std::string& returnBtnName = "PlayReturn";
  const std::string& tickTextName = "PlayTick";
  const std::string& tpsTextName = "PlayTPS";
  const std::string& fpsTextName = "PlayFPS";
  const std::string& activeBodiesTextName = "PlayActiveBodies";
  const std::string& playEntityCountTextName = "PlayEntityCount";

  public:
  PlayState(EntityWorld& world) : m_world(world) {
    m_world.component<OwnedBy<PlayState>>();
    m_addedPlayerComponents = m_world.set<PlayerComponent, PrimaryTargetTag, BodyComponent,
                                          RectangleRenderComponent, InventoryComponent>();

    tgui::Gui& gui = m_world.getContext<tgui::Gui>();
    m_menu = world.getContext<tgui::Gui>().get(menuName);
    tgui::Button::Ptr returnBtn = gui.get(returnBtnName)->cast<tgui::Button>();

    returnBtn->onMousePress([&]() {
      StateManager& stateMgr = world.getContext<StateManager>();
      stateMgr.disableState("PlayState");
      stateMgr.enableState("MenuState");
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
      Vec2 mousePos = m_world.getContext<Render>().getWorldCoords(m_world.getContext<EventManager>().getMouseCoords());

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
    StateManager& stateMgr = m_world.getContext<StateManager>();
    b2WorldId physicsWorld = m_world.getContext<b2WorldId>();
    InventoryManager& invMgr = m_world.getContext<InventoryManager>();
    AssetLoader& assetLoader = m_world.getContext<AssetLoader>();
    Entity player = m_world.getFirstWith(m_world.set<CameraInUse>());

    m_menu->setEnabled(true);
    m_menu->setVisible(true);

    /* Needed Modules */
    m_world.enableModule<PhysicsModule>();
    m_world.enableModule<PathfindingModule>();
    m_world.enableModule<PlayerModule>();

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
    RefT<RectangleRenderComponent> renderRect = player.get<RectangleRenderComponent>();
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

    RefT<Transform> playerTransform = player.get<TransformComponent>();
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
    AppStats& appStats = m_world.getContext<AppStats>();
    b2WorldId& physicsWorldId = m_world.getContext<b2WorldId>();
    u32 activeBodies = b2World_GetAwakeBodyCount(physicsWorldId);

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

    b2WorldId physicsWorld = m_world.getContext<b2WorldId>();
    if (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_F3] && tick % 3 == 0) {
      callInGrid(-2, -2, 2, 2, m_bodyCallback);
    }
  }

  void onLeave() {
    Entity player = m_world.getFirstWith(m_world.set<CameraComponent>());

    m_world.destroyAllEntitiesWith(m_world.set<OwnedBy<PlayState>, EntityWorld::Enabled>());
    player.remove(m_addedPlayerComponents);
    m_world.disableModule<PhysicsModule>();
    m_world.disableModule<PathfindingModule>();
    m_world.disableModule<PlayerModule>();

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
