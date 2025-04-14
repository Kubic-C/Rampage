#pragma once
#include "transform.hpp"
#include "physics.hpp"
#include "render/render.hpp"

struct PlayerComponent {
  glm::vec2 mouse = { 0, 0 };
  float accel = 1.0f;
  float maxSpeed = 5.0f;
};

class PlayerModule : public Module {
public:
  PlayerModule(EntityWorld& world) 
    :  m_system(world.system(world.set<BodyComponent, RotComponent, PlayerComponent, CameraComponent>(), &updatePlayer)) {
  }
  
  void run(EntityWorld& world, float deltaTime) override {
    m_system.run();
  }

  static void updatePlayer(Entity e, float deltaTime) {
    EntityWorld& world = e.world();
    SDL_Window* window = world.getContext<SDL_Window*>();
    Render& render = world.getContext<Render>();
    BodyComponent& body = e.get<BodyComponent>();
    RotComponent& rot = e.get<RotComponent>();
    PlayerComponent& player = e.get<PlayerComponent>();
    CameraComponent& camera = e.get<CameraComponent>();

    float mass = b2Body_GetMass(body.id);
    const bool* keyboard = SDL_GetKeyboardState(nullptr);
    if (keyboard[SDL_SCANCODE_A])
      b2Body_ApplyLinearImpulseToCenter(body.id, fast2DRotate({ mass * -player.accel, 0.0f }, camera.m_rot), true);
    if (keyboard[SDL_SCANCODE_D])
      b2Body_ApplyLinearImpulseToCenter(body.id, fast2DRotate({ mass * player.accel, 0.0f }, camera.m_rot), true);
    if (keyboard[SDL_SCANCODE_W])
      b2Body_ApplyLinearImpulseToCenter(body.id, fast2DRotate({0.0f, mass * player.accel}, camera.m_rot), true);
    if (keyboard[SDL_SCANCODE_S])
      b2Body_ApplyLinearImpulseToCenter(body.id, fast2DRotate({ 0.0f, mass * -player.accel }, camera.m_rot), true);
    if (keyboard[SDL_SCANCODE_Z])
      camera.safeZoom(10);
    if (keyboard[SDL_SCANCODE_X])
      camera.safeZoom(-10);
    if (keyboard[SDL_SCANCODE_Q])
      camera.m_rot += 0.1f;
    if (keyboard[SDL_SCANCODE_E])
      camera.m_rot -= 0.1f;

    float maxSpeed = player.maxSpeed;
    if (keyboard[SDL_SCANCODE_LSHIFT])
      maxSpeed *= 2;

    glm::vec2 vel = (Vec2)b2Body_GetLinearVelocity(body.id);
    if (glm::length(vel) > maxSpeed) {
      vel = glm::normalize(vel) * maxSpeed; 
      b2Body_SetLinearVelocity(body.id, (Vec2)vel);
    }

    {
      float x, y;
      SDL_GetMouseState(&x, &y);
      player.mouse = render.getWorldCoords({x, y});
    }

    glm::vec2 dir = glm::normalize((Vec2)b2Body_GetPosition(body.id) - player.mouse);
    float angle = atan2(dir.y, dir.x);
    rot = Rot(angle);
  }

private:
  System m_system;
};
