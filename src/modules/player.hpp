#pragma once

#include "../components/player.hpp"
#include "../components/tilemap.hpp"
#include "../components/worldMap.hpp"

#include "../render/render.hpp"

#include "../eventManager.hpp"
#include "../inventory.hpp"

class PlayerModule : public Module {
  public:
  static void registerComponents(EntityWorld& world) {
    world.set<BodyComponent, TransformComponent, PlayerComponent, CameraComponent, InventoryComponent>();
  }

  PlayerModule(EntityWorld& world) :
      m_system(world.system(world.set<BodyComponent, TransformComponent, PlayerComponent, CameraComponent,
                                      InventoryComponent>(),
                            &updatePlayer)) {}

  void run(EntityWorld& world, float deltaTime) override { m_system.run(); }

  static void updatePlayer(Entity e, float deltaTime) {
    EntityWorld& world = e.world();
    SDL_Window* window = world.getContext<SDL_Window*>();
    Render& render = world.getContext<Render>();
    InventoryManager& invMgr = world.getContext<InventoryManager>();
    EventManager& eventMgr = world.getContext<EventManager>();
    AssetLoader& assetLoader = world.getContext<AssetLoader>();

    RefT<BodyComponent> body = e.get<BodyComponent>();
    RefT<TransformComponent> transform = e.get<TransformComponent>();
    RefT<PlayerComponent> player = e.get<PlayerComponent>();
    RefT<CameraComponent> camera = e.get<CameraComponent>();
    Inventory inv = invMgr.getInventory(e.get<InventoryComponent>()->id);

    /* Linear Movement */
    float mass = b2Body_GetMass(body->id);
    if (eventMgr.isKeyHeld(Key::A))
      b2Body_ApplyLinearImpulseToCenter(body->id, fast2DRotate({mass * -player->accel, 0.0f}, camera->m_rot),
                                        true);
    if (eventMgr.isKeyHeld(Key::D))
      b2Body_ApplyLinearImpulseToCenter(body->id, fast2DRotate({mass * player->accel, 0.0f}, camera->m_rot),
                                        true);
    if (eventMgr.isKeyHeld(Key::W))
      b2Body_ApplyLinearImpulseToCenter(body->id, fast2DRotate({0.0f, mass * player->accel}, camera->m_rot),
                                        true);
    if (eventMgr.isKeyHeld(Key::S))
      b2Body_ApplyLinearImpulseToCenter(body->id, fast2DRotate({0.0f, mass * -player->accel}, camera->m_rot),
                                        true);

    float maxSpeed = player->maxSpeed;
    if (eventMgr.isKeyHeld(Key::LeftShift))
      maxSpeed *= 8;
    glm::vec2 vel = (Vec2)b2Body_GetLinearVelocity(body->id);
    if (glm::length(vel) > maxSpeed) {
      vel = glm::normalize(vel) * maxSpeed;
      b2Body_SetLinearVelocity(body->id, (Vec2)vel);
    }

    /* Zoom */
    if (eventMgr.isKeyHeld(Key::Z))
      camera->safeZoom(10);
    if (eventMgr.isKeyHeld(Key::X))
      camera->safeZoom(-10);

    /* Camera Rotation */
    if (eventMgr.isKeyHeld(Key::Q))
      camera->m_rot += 0.1f;
    if (eventMgr.isKeyHeld(Key::E))
      camera->m_rot -= 0.1f;

    /* Cursor */
    Vec2 mouseScreen = eventMgr.getMouseCoords();
    player->mouse = render.getWorldCoords(mouseScreen);
    glm::vec2 dir = glm::normalize((Vec2)b2Body_GetPosition(body->id) - player->mouse);
    transform->rot = Rot(atan2(dir.y, dir.x));

    /* Inventory Stuff */
    if (eventMgr.isKeyPressed(Key::Tab))
      inv.setVisible(!inv.getVisible());
    if (eventMgr.isKeyHeld(Key::F4))
      inv.addItem(assetLoader.getPrefab("WoodItem"), 4);
    if (eventMgr.isKeyHeld(Key::F) && invMgr.isHandEmpty() &&
        !world.getContext<tgui::Gui>().getWidgetAtPos(mouseScreen, false))
      tryPlaceItem(world.getFirstWith(world.set<WorldMapTag>()), invMgr.getHandInventory(),
                   invMgr.getHandInventoryPos(), player->mouse);
  }

  private:
  System m_system;
};
