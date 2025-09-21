#pragma once

#include "../../common/common.hpp"

RAMPAGE_START

enum PhysicsCategories { Friendly = 0x01, Enemy = 0x02, Static = 0x04, All = 0xFFFF };

struct BodyComponent {
  b2BodyId id = b2_nullBodyId;

  BodyComponent() = default;
  BodyComponent(const BodyComponent& body) = default;

  BodyComponent(BodyComponent&& other) noexcept {
    id = other.id;
    other.id = b2_nullBodyId;
  }

  BodyComponent& operator=(BodyComponent&& other) noexcept {
    id = other.id;
    other.id = b2_nullBodyId;

    return *this;
  }

  ~BodyComponent() {
    if (b2Body_IsValid(id)) {
      b2DestroyBody(id);
    }
  }
};

enum AddShapeType { Unknown, Circle, Polygon };

using ShapeVariant = std::variant<b2Circle, b2Polygon>;

struct AddShapeComponent {
  b2ShapeDef def = b2DefaultShapeDef();
  ShapeVariant shape;
};


struct AddBodyComponent : public b2BodyDef {
  AddBodyComponent() { dynamic_cast<b2BodyDef&>(*this) = b2DefaultBodyDef(); }

  AddBodyComponent(glz::make_reflectable) { dynamic_cast<b2BodyDef&>(*this) = b2DefaultBodyDef(); }
};

RAMPAGE_END

template <>
struct glz::meta<b2BodyType> {
  static constexpr auto value =
      enumerate("static", b2_staticBody, "kinematic", b2_kinematicBody, "dynamic", b2_dynamicBody);
};

template <>
struct glz::meta<rmp::AddBodyComponent> {
  using T = b2BodyDef;
  static constexpr auto value =
      object("bodyType", &T::type, "linearVelocity", &T::linearVelocity, "angularVelocity",
             &T::angularVelocity, "linearDamping", &T::linearDamping, "angularDamping", &T::angularDamping,
             "gravityScale", &T::gravityScale, "sleepThreshold", &T::sleepThreshold, "enableSleep",
             &T::enableSleep, "isAwake", &T::isAwake, "fixedRotation", &T::fixedRotation, "isBullet",
             &T::isBullet, "isEnabled", &T::isEnabled, "allowFastRotation", &T::allowFastRotation);
};

template <>
struct glz::meta<b2SurfaceMaterial> {
  using T = b2SurfaceMaterial;
  static constexpr auto value = object(
      "friction", &T::friction, "restitution", &T::restitution, "rollingResistance", &T::rollingResistance,
      "tangentSpeed", &T::tangentSpeed, "userMaterialId", &T::userMaterialId, "customColor", &T::customColor);
};

template <>
struct glz::meta<b2Filter> {
  using T = b2Filter;
  static constexpr auto value =
      object("categoryBits", &T::categoryBits, "maskBits", &T::maskBits, "groupIndex", &T::groupIndex);
};

template <>
struct glz::meta<b2ShapeDef> {
  using T = b2ShapeDef;
  static constexpr auto value =
      object("material", &T::material, "density", &T::density, "isSensor", &T::isSensor, "enableSensorEvents",
             &T::enableSensorEvents, "enableContactEvents", &T::enableContactEvents, "enableHitEvents",
             &T::enableHitEvents, "enablePreSolveEvents", &T::enablePreSolveEvents, "invokeContactCreation",
             &T::invokeContactCreation, "updateBodyMass", &T::updateBodyMass);
};

template <>
struct glz::meta<b2Circle> {
  using T = b2Circle;
  static constexpr auto value = object("radius", &T::radius);
};

template <>
struct glz::meta<b2Vec2> {
  using T = b2Vec2;
  static constexpr auto value = object("x", &T::x, "y", &T::y);
};

template <>
struct glz::meta<b2Polygon> {
  using T = b2Polygon;
  static constexpr auto value = object("vertices", &T::vertices, "normals", &T::normals, "centroid",
                                       &T::centroid, "radius", &T::radius, "count", &T::count);
};

template <>
struct glz::meta<rmp::ShapeVariant> {
  static constexpr std::string_view tag = "shapeType";
  static constexpr auto ids = std::array{"circle", "polygon"};
};