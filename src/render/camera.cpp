#include "camera.hpp"

RAMPAGE_START

void CameraComponent::serialize(capnp::MessageBuilder& builder, Ref component) {
  auto cameraBuilder = builder.initRoot<Schema::CameraComponent>();
  auto camera = component.cast<CameraComponent>();

  cameraBuilder.setZoom(camera->m_zoom);
  cameraBuilder.setRot(camera->m_rot);
}

void CameraComponent::deserialize(capnp::MessageReader& reader, Ref component) {
  auto cameraReader = reader.getRoot<Schema::CameraComponent>();
  auto camera = component.cast<CameraComponent>();

  camera->m_zoom = cameraReader.getZoom();
  camera->m_rot  = cameraReader.getRot();
}

void CameraComponent::fromJson(Ref component, AssetLoader loader, const json& compJson) {
  auto camera = component.cast<CameraComponent>();

  if(compJson.contains("zoom") && compJson["zoom"].is_number())
    camera->m_zoom = compJson["zoom"];
  if(compJson.contains("rot") && compJson["rot"].is_number())
    camera->m_rot = compJson["rot"];
}

CameraComponent::CameraComponent() : m_zoom(100.0f), m_rot(0) {}

glm::mat4 CameraComponent::view(const b2Transform& transform, const Vec2& screenDim, float z) const {
  Vec2 cameraPos = Vec2(transform.p);
  
  glm::mat4 view = glm::identity<glm::mat4>();
  view = glm::lookAt(glm::vec3(cameraPos, z), glm::vec3(cameraPos, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
  view = glm::ortho<float>(-1.0f, 1.0f, -1.0f, 1.0f, 1000.0f, -1000.0f) * view;
  view = glm::rotate(view, -Rot(transform.q).radians(), glm::vec3(0.0f, 0.0f, 1.0f));
  view = glm::scale(view, glm::vec3(glm::vec2(m_zoom), 1.0f));
  view = glm::translate(view, glm::vec3(-cameraPos, 0.0f));

  return view;
}

void CameraComponent::safeZoom(float amount) {
  if (m_zoom + amount < 0.01f)
    return;
  m_zoom += amount;
}

RAMPAGE_END