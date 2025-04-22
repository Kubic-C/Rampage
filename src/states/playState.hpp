#pragma once

#include "state.hpp"
#include "../turret.hpp"

class PlayState : public State {
  const std::string& menuName = "PlayMenu";
  const std::string& returnBtnName = "PlayReturn";
  const std::string& tickTextName = "PlayTick";
  const std::string& tpsTextName = "PlayTPS";
  const std::string& fpsTextName = "PlayFPS";
  const std::string& activeBodiesTextName = "PlayActiveBodies";
  const std::string& playEntityCountTextName = "PlayEntityCount";
public:
  PlayState(EntityWorld& world)
    : m_world(world) {
    m_world.component<OwnedBy<PlayState>>();
    m_addedPlayerComponents = m_world.set<PlayerComponent, PrimaryTargetTag, BodyComponent, RectangleRenderComponent, InventoryComponent>();

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

    b2WorldId& physicsWorldId = m_world.getContext<b2WorldId>();
    m_bodyCallback =
      [&](int x, int y) {
      Entity seeker = m_world.create();
      seeker.add<TransformComponent>();
      seeker.add<BodyComponent>();
      seeker.add<SeekPrimaryTargetTag>();
      seeker.add<CircleRenderComponent>();
      seeker.add<OwnedBy<PlayState>>();
      seeker.add<HealthComponent>();
      seeker.add<ContactDamageComponent>();

      ContactDamageComponent& damage = seeker.get<ContactDamageComponent>();
      damage.damage = 12;

      seeker.add<SubmitToCollisionQueueComponent>();
      seeker.get<SubmitToCollisionQueueComponent>().queue = world.getModule<PathfindingModule>().getContactDamageQueue();

      CircleRenderComponent& circleRender = seeker.get<CircleRenderComponent>();
      circleRender.radius = 0.1f;

      TransformComponent& transform = seeker.get<TransformComponent>();
      transform.pos = { x * 0.3f - 7, y * 0.3f + 14 };
      transform.rot = Rot(0);
      b2BodyDef bodyDef = b2DefaultBodyDef();
      bodyDef.type = b2_dynamicBody;
      bodyDef.position = Vec2(0, 0);
      bodyDef.linearDamping = 10;
      b2ShapeDef shapeDef = b2DefaultShapeDef();
      shapeDef.friction = 0;
      shapeDef.filter.categoryBits = Enemy;
      shapeDef.filter.maskBits = ~Enemy; // everything but ourselves
      shapeDef.enableContactEvents = true;
      shapeDef.userData = entityToB2Data(seeker);
      b2Circle circle;
      circle.radius = 0.1f;
      circle.center = Vec2(0);
      b2BodyId bodyId = b2CreateBody(physicsWorldId, &bodyDef);
      seeker.get<BodyComponent>().id = bodyId;
      b2CreateCircleShape(bodyId, &shapeDef, &circle);
      };
  }

  void onEntry() {
    b2WorldId physicsWorld = m_world.getContext<b2WorldId>();
    ItemManager& itemMgr = m_world.getContext<ItemManager>();
    Entity player = m_world.getFirstWith(m_world.set<CameraInUse>());

    m_menu->setEnabled(true);
    m_menu->setVisible(true);

    /* Needed Modules */
    m_world.enableModule<PhysicsModule>();
    m_world.enableModule<PathfindingModule>();
    m_world.enableModule<PlayerModule>();

    /* Player */
    player.add(m_addedPlayerComponents);
    Inventory playerInvetory = itemMgr.createInventory("Player Inventory", 3, 5);
    player.get<InventoryComponent>().id = playerInvetory;
    playerInvetory.addItem(itemMgr.getItem("BasicTurretItem"), 20);
    playerInvetory.addItem(itemMgr.getItem("PlacableHighStoneItem"), 1);
    playerInvetory.addItem(itemMgr.getItem("WoodItem"), 32);

    // Render
    RectangleRenderComponent& renderRect = player.get<RectangleRenderComponent>();
    renderRect.hw = 0.12f;
    renderRect.hh = 0.12f;

    // Collider
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.fixedRotation = true;
    bodyDef.linearDamping = 10;
    b2BodyId bodyId = b2CreateBody(physicsWorld, &bodyDef);
    player.get<BodyComponent>().id = bodyId;
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1000;
    b2Polygon rect = b2MakeBox(0.12f, 0.12f);
    b2CreatePolygonShape(bodyId, &shapeDef, &rect);

    Transform& playerTransform = player.get<TransformComponent>();
    playerTransform.pos = { 5.0f, -7.0f };

    /* WorldMap & Tilemap Component */

    { // World Map
      Entity tm = m_world.create();
      tm.add<TransformComponent>();
      tm.add<BodyComponent>();
      tm.add<TilemapComponent>();
      tm.add<WorldMapTag>();
      tm.add<OwnedBy<PlayState>>();

      TransformComponent& transform = tm.get<TransformComponent>();
      transform.pos = { 0, 0 };
      transform.rot = Rot(0);
      b2BodyDef bodyDef = b2DefaultBodyDef();
      bodyDef.type = b2_staticBody;
      bodyDef.position = Vec2(0, 0);
      b2BodyId bodyId = b2CreateBody(physicsWorld, &bodyDef);
      tm.get<BodyComponent>().id = bodyId;
      
      TilePrefabs& tilePrefabs = m_world.getContext<TilePrefabs>();
      TilemapComponent& tilemapLayers = tm.get<TilemapComponent>();
      Tilemap& worldLayer = tilemapLayers.getTilemap(TilemapWorldLayer);
      auto tileCallback =
        [&](int x, int y) {
          if (x <= -35 || x >= 35 || y <= -35 || y >= 35 || (x % 5 == 0 && y < 20 && y != -34)) {
            TileDef highStone = tilePrefabs.clonePrefab("PermaHighStone");
            worldLayer.insert(m_world, bodyId, { x, y }, tm, highStone);
          }
          else if (rand() % 100 < 5) {
            TileDef unknown = tilePrefabs.clonePrefab("Unknown");
            worldLayer.insert(m_world, bodyId, { x, y }, tm, unknown);
          } else {
            TileDef stone = tilePrefabs.clonePrefab("StoneFloor");
            worldLayer.insert(m_world, bodyId, { x, y }, tm, stone);
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
    m_entityCountText->setText("Entity Count: " + std::to_string(m_world.getEntityCount()));

    b2WorldId physicsWorld = m_world.getContext<b2WorldId>();
    if (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_F3]) {
      callInGrid(-2, -2, 2, 2, m_bodyCallback);
    }
  }

  void onLeave() {
    Entity player = m_world.getFirstWith(m_world.set<CameraComponent>());

    m_world.destroyAllEntitiesWith(m_world.set<OwnedBy<PlayState>>());
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