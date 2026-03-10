#include "camera.hpp"

RAMPAGE_START

void CameraComponent::serialize(capnp::MessageBuilder& builder, Ref component) {
  auto cameraBuilder = builder.initRoot<Schema::CameraComponent>();
  auto camera = component.cast<CameraComponent>();

  cameraBuilder.setZoom(camera->m_zoom);
  cameraBuilder.setRot(camera->m_rot);
}

void CameraComponent::deserialize(capnp::MessageReader& reader, const IdMapper& idMapper, Ref component) {
  auto cameraReader = reader.getRoot<Schema::CameraComponent>();
  auto camera = component.cast<CameraComponent>();

  camera->m_zoom = cameraReader.getZoom();
  camera->m_rot  = cameraReader.getRot();
}

void CameraComponent::fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& jsonValue) {
  auto camera = component.cast<CameraComponent>();
  auto compJson = jsonValue.as<JSchema::CameraComponent>();

  if(compJson->hasZoom())
    camera->m_zoom = compJson->getZoom();
  if(compJson->hasRot())
    camera->m_rot = compJson->getRot();
}

CameraComponent::CameraComponent() : m_zoom(100.0f), m_rot(0) {}

glm::mat4 CameraComponent::view(const Transform& transform, const Vec2& screenDim, float z) const {
  Vec2 cameraPos = Vec2(transform.pos);

  glm::mat4 view = glm::identity<glm::mat4>();
  view = glm::rotate(view, -transform.rot.radians(), glm::vec3(0.0f, 0.0f, 1.0f));
  view = glm::scale(view, glm::vec3(glm::vec2(m_zoom), 1.0f));
  view = glm::translate(view, glm::vec3(-cameraPos, 0.0f));
  view = glm::ortho<float>(-1.0f, 1.0f, -1.0f, 1.0f, 1000.0f, -1000.0f) * view;

  return view;
}

ViewRect CameraComponent::getViewRect(const Transform& transform, const Vec2& screenDim, float z) const {
  Vec2 cameraPos = Vec2(transform.pos);
  float aspectRatio = screenDim.x / screenDim.y;
  Vec2 halfDim = Vec2(screenDim.y * 0.5f * aspectRatio, screenDim.y * 0.5f) / m_zoom;
  return ViewRect{cameraPos, halfDim};
}

void CameraComponent::safeZoom(float amount) {
  if (m_zoom + amount < 0.01f)
    return;
  m_zoom += amount;
}

RAMPAGE_END