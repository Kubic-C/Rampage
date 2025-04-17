#pragma once
#include "transform.hpp"
#include "physics.hpp"
#include "render/render.hpp"

#include "tilemap.hpp"
#include "item.hpp"
#include "worldMap.hpp"
#include "eventManager.hpp"

struct PlayerComponent {
  glm::vec2 mouse = { 0, 0 };
  float accel = 1.0f;
  float maxSpeed = 5.0f;
};

class PlayerModule : public Module {
public:
  PlayerModule(EntityWorld& world) 
    :  m_system(world.system(world.set<BodyComponent, TransformComponent, PlayerComponent, CameraComponent, InventoryComponent>(), &updatePlayer)) {
  }
  
  void run(EntityWorld& world, float deltaTime) override {
    m_system.run();
  }

  static void updatePlayer(Entity e, float deltaTime) {
    EntityWorld& world = e.world();
    SDL_Window* window = world.getContext<SDL_Window*>();
    Render& render = world.getContext<Render>();
    ItemManager& itemMgr = world.getContext<ItemManager>();
    EventManager& eventMgr = world.getContext<EventManager>();

    BodyComponent& body = e.get<BodyComponent>();
    TransformComponent& transform = e.get<TransformComponent>();
    PlayerComponent& player = e.get<PlayerComponent>();
    CameraComponent& camera = e.get<CameraComponent>();
    Inventory inv = itemMgr.getInventory(e.get<InventoryComponent>().id);

    float mass = b2Body_GetMass(body.id);
    if (eventMgr.isKeyHeld(Key::A))
      b2Body_ApplyLinearImpulseToCenter(body.id, fast2DRotate({ mass * -player.accel, 0.0f }, camera.m_rot), true);
    if (eventMgr.isKeyHeld(Key::D))
      b2Body_ApplyLinearImpulseToCenter(body.id, fast2DRotate({ mass * player.accel, 0.0f }, camera.m_rot), true);
    if (eventMgr.isKeyHeld(Key::W))
      b2Body_ApplyLinearImpulseToCenter(body.id, fast2DRotate({0.0f, mass * player.accel}, camera.m_rot), true);
    if (eventMgr.isKeyHeld(Key::S))
      b2Body_ApplyLinearImpulseToCenter(body.id, fast2DRotate({ 0.0f, mass * -player.accel }, camera.m_rot), true);
    if (eventMgr.isKeyHeld(Key::Z))
      camera.safeZoom(10);
    if (eventMgr.isKeyHeld(Key::X))
      camera.safeZoom(-10);
    if (eventMgr.isKeyHeld(Key::Q))
      camera.m_rot += 0.1f;
    if (eventMgr.isKeyHeld(Key::E))
      camera.m_rot -= 0.1f;
    if (eventMgr.isKeyPressed(Key::Tab)) {
      inv.setVisible(!inv.getVisible());
      logGeneric("inv: %i\n", inv.getVisible());
    }

    float maxSpeed = player.maxSpeed;
    if (eventMgr.isKeyHeld(Key::E))
      maxSpeed *= 8;

    glm::vec2 vel = (Vec2)b2Body_GetLinearVelocity(body.id);
    if (glm::length(vel) > maxSpeed) {
      vel = glm::normalize(vel) * maxSpeed; 
      b2Body_SetLinearVelocity(body.id, (Vec2)vel);
    }

    {
      float x, y;
      SDL_MouseButtonFlags flags = SDL_GetMouseState(&x, &y);
      player.mouse = render.getWorldCoords({x, y});

      tgui::Gui& gui = world.getContext<tgui::Gui>();
      ItemManager& itemMgr = world.getContext<ItemManager>();
      if (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_F] && itemMgr.getHandInventory() != 0 && !gui.getWidgetAtPos({x, y}, false)) {
        Render& renderer = world.getContext<Render>();
        tryPlaceItem(world.getFirstWith(world.set<WorldMapTag>()), itemMgr.getInventory(itemMgr.getHandInventory()), itemMgr.getHandInventoryPos(), renderer.getWorldCoords({x, y}));
      }
    }

    glm::vec2 dir = glm::normalize((Vec2)b2Body_GetPosition(body.id) - player.mouse);
    float angle = atan2(dir.y, dir.x);
    transform.rot = Rot(angle);
  }

private:
  System m_system;
};
