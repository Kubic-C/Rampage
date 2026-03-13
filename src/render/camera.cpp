#include "camera.hpp"

RAMPAGE_START

void CameraComponent::serialize(capnp::MessageBuilder& builder, Ref component) {
  auto cameraBuilder = builder.initRoot<Schema::CameraComponent>();
  auto camera = component.cast<CameraComponent>();

  cameraBuilder.setZoom(camera->zoom);
  cameraBuilder.setRot(camera->rot);
}

void CameraComponent::deserialize(capnp::MessageReader& reader, const IdMapper& idMapper, Ref component) {
  auto cameraReader = reader.getRoot<Schema::CameraComponent>();
  auto camera = component.cast<CameraComponent>();

  camera->zoom = cameraReader.getZoom();
  camera->rot = cameraReader.getRot();
}

void CameraComponent::fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& jsonValue) {
  auto camera = component.cast<CameraComponent>();
  auto compJson = jsonValue.as<JSchema::CameraComponent>();

  if(compJson->hasZoom())
    camera->zoom = compJson->getZoom();
  if(compJson->hasRot())
    camera->rot = compJson->getRot();
}

RAMPAGE_END