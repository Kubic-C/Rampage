#pragma once

#include "../../event/module.hpp"
#include "../../render/module.hpp"
#include "../components/components.hpp"
#include "state.hpp"
#include "../../common/ecs/taggedWorld.hpp"

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
                                          RectangleRenderComponent, InventoryComponent>();

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
      auto& stateMgr = m_world->getContext<StateManager>();
    });

    tgui::Button::Ptr loadStateBtn = gui.get(playLoadStateTextName)->cast<tgui::Button>();
    loadStateBtn->onMousePress([=]() {
      auto& stateMgr = m_world->getContext<StateManager>();
    });

    m_tickText = gui.get(tickTextName)->cast<tgui::Label>();
    m_tpsText = gui.get(tpsTextName)->cast<tgui::Label>();
    m_fpsText = gui.get(fpsTextName)->cast<tgui::Label>();
    m_activeBodiesText = gui.get(activeBodiesTextName)->cast<tgui::Label>();
    m_entityCountText = gui.get(playEntityCountTextName)->cast<tgui::Label>();

    EntityPtr base = m_world->create();
    base.disable();

    m_bodyCallback = [this, baseId = base.id()](int x, int y) {
      Vec2 mousePos = m_world->getContext<RenderModule>().getWorldCoords(
          m_world->getContext<EventModule>().getMouseCoords());

      auto& loader = m_world->getContext<AssetLoader>();

      std::string enemyType = "BasicZombie";
      if (rand() % 20 < 1)
        enemyType = "BigAssZombie";

      EntityPtr seeker = m_world->clone(loader.getPrefabId(enemyType));
      m_world->getEntity(baseId).copyInto(seeker);
      seeker.get<TransformComponent>()->pos = mousePos + Vec2(x * 0.3f, y * 0.3f) - Vec2(2 * 0.3f, 2 * 0.3f);
    };
  }

  void onEntry() {
    const b2WorldId physicsWorld = m_world->getContext<b2WorldId>();
    auto& stateMgr = m_world->getContext<StateManager>();
    auto& invMgr = m_world->getContext<InventoryManager>();
    auto& assetLoader = m_world->getContext<AssetLoader>();
    EntityPtr player = m_world->getFirstWith(m_world->set<CameraInUseTag>());

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
    bodyDef.linearDamping = 10;
    b2BodyId bodyId = b2CreateBody(physicsWorld, &bodyDef);
    player.get<BodyComponent>()->id = bodyId;
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1000;
    b2Polygon rect = b2MakeBox(0.12f, 0.12f);
    b2CreatePolygonShape(bodyId, &shapeDef, &rect);

    RefT<TransformComponent> playerTransform = player.get<TransformComponent>();
    playerTransform->pos = Vec2(0.0f, 0.0f);

    /* WorldMap & Tilemap Component */

{
    // World Map
    EntityPtr tm = m_world->create();
    tm.add<TransformComponent>();
    tm.add<BodyComponent>();
    tm.add<TilemapComponent>();
    tm.add<WorldMapTag>();

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

    const int gridSize = 3;
    const int roomRadius = 6;
    const int spacing = 13; // distance between room centers

    // Precompute room centers
    std::vector<Vec2> centers;
    for (int gx = 0; gx < gridSize; gx++) {
        for (int gy = 0; gy < gridSize; gy++) {
            int cx = (gx - gridSize/2) * spacing;
            int cy = (gy - gridSize/2) * spacing;
            centers.push_back(Vec2(cx, cy));
        }
    }

    auto isInsideRoom = [&](int x, int y) {
        for (auto& c : centers) {
            int dx = x - c.x;
            int dy = y - c.y;
            if (dx*dx + dy*dy <= roomRadius * roomRadius)
                return true;
        }
        return false;
    };

    auto isInCorridor = [&](int x, int y) {
        for (auto& c : centers) {
            // connect right neighbor
            Vec2 right = {c.x + spacing, c.y};
            if (std::find(centers.begin(), centers.end(), right) != centers.end()) {
                if ( (y >= std::min(c.y, right.y) - 2 && y <= std::max(c.y, right.y) + 2) &&
                     (x >= std::min(c.x, right.x) && x <= std::max(c.x, right.x)) ) {
                    return true;
                }
            }
            // connect down neighbor
            Vec2 down = {c.x, c.y + spacing};
            if (std::find(centers.begin(), centers.end(), down) != centers.end()) {
                if ( (x >= std::min(c.x, down.x) - 2 && x <= std::max(c.x, down.x) + 2) &&
                     (y >= std::min(c.y, down.y) && y <= std::max(c.y, down.y)) ) {
                    return true;
                }
            }
        }
        return false;
    };

    auto tileCallback = [&](int x, int y) {
        if (isInsideRoom(x, y) || isInCorridor(x, y)) {
            if (rand() % 100 < 5) {
                TileDef unknown = assetLoader.cloneTilePrefab("UnknownTile");
                worldLayer.insert(m_world, bodyId, {x, y}, tm, unknown);
            } else {
                TileDef stone = assetLoader.cloneTilePrefab("StoneFloorTile");
                worldLayer.insert(m_world, bodyId, {x, y}, tm, stone);
            }
        } else {
          TileDef stone = assetLoader.cloneTilePrefab("PermaHighStoneTile");
          worldLayer.insert(m_world, bodyId, {x, y}, tm, stone);
        }
    };

    // Scan enough area to cover all rooms + corridors
    int bound = gridSize * spacing / 2 + roomRadius + 5;
    callInGrid(-bound, -bound, bound, bound, tileCallback);
}

    EntityPtr e = m_world->create();
    e.add<TransformComponent>();
    e.add<SpawnerComponent>();

    auto spawner = e.get<SpawnerComponent>();
    spawner->spawn = assetLoader.getPrefab("BasicZombie");
    spawner->spawnableRadius = 2.5f;
    spawner->spawnCount = 100;
    spawner->spawnRate = 0.5f;
    spawner->timeSinceLastSpawn = 25.0f;
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

    const b2WorldId physicsWorld = m_world->getContext<b2WorldId>();
    if (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_F3] && tick % 3 == 0) {
      callInGrid(-2, -2, 2, 2, m_bodyCallback);
    }
  }

  void onLeave() {
    EntityPtr player = m_world->getFirstWith(m_world->set<CameraComponent>());

    m_world->destroyAllEntitiesWith(m_world->set<OwnedBy<PlayState>, IWorld::Enabled>());
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
  IWorldPtr m_world;
};

RAMPAGE_END
