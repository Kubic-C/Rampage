#include "player.hpp"

#include "../../event/module.hpp"
#include "../../render/camera.hpp"
#include "../../render/module.hpp"
#include "../module.hpp"

#include "../components/player.hpp"
#include "../components/tilemap.hpp"
#include "../components/worldMap.hpp"
#include "../systems/inventory.hpp"

RAMPAGE_START

void updatePlayer(EntityPtr e, float dt) {
  IWorldPtr world = e.world();
  auto& render = world->getContext<RenderModule>();
  auto& eventMgr = world->getContext<EventModule>();
  auto& invMgr = world->getContext<InventoryManager>();
  auto assetLoader = world->getAssetLoader();

  auto body = e.get<BodyComponent>();
  auto transform = e.get<TransformComponent>();
  auto player = e.get<PlayerComponent>();
  auto camera = e.get<CameraComponent>();

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

  u32 contactCap = b2Body_GetContactCapacity(body->id);
  std::vector<b2ContactData> contacts(contactCap);
  int contactCount = b2Body_GetContactData(body->id, contacts.data(), contactCap);
  contacts.resize(contactCount);
  for(b2ContactData& contact : contacts) {
    EntityBox2dUserData* otherUserData = getEntityBox2dUserData(contact.shapeIdA);
    if (!otherUserData || otherUserData->entity == e)
      otherUserData = getEntityBox2dUserData(contact.shapeIdB);
    if (!otherUserData)
      continue;

    EntityPtr other = world->getEntity(otherUserData->entity);
    if(other.has<ItemStackComponent>() && !other.has<ItemPlacedTag>()) {
      auto itemStackComp = other.get<ItemStackComponent>();

      u32 amountNotAdded = invMgr.addItem(world, e, itemStackComp->itemId, itemStackComp->count);
      if(amountNotAdded == 0) {
        world->destroy(other);
      } else {
        itemStackComp->count = amountNotAdded;
      }
    }
  }
}

int updatePlayers(IWorldPtr world, float dt) {
  auto it = world->getWith(
      world->set<BodyComponent, TransformComponent, PlayerComponent, CameraComponent>());
  while (it->hasNext())
    updatePlayer(it->next(), dt);

  return 0;
}

void loadPlayerSystems(IHost& host) {
  Pipeline& pipeline = host.getPipeline();

  pipeline.getGroup<GameGroup>().attachToStage<GameGroup::TickStage>(updatePlayers);
}

RAMPAGE_END
