#pragma once

#include "../utility/base.hpp"

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
          logGeneric("Destroy body\n");
        }
    }
};

enum AddShapeType {
  Unknown,
  Circle,
  Polygon
};

struct AddShapeComponent {
  using ShapeVariant = std::variant<b2Circle, b2Polygon>;

  b2ShapeDef def = b2DefaultShapeDef();
  ShapeVariant shape;
};

template <>
struct glz::meta<b2SurfaceMaterial> {
  using T = b2SurfaceMaterial;
  static constexpr auto value = object(
    "friction", &T::friction,
    "restitution", &T::restitution,
    "rollingResistance", &T::rollingResistance,
    "tangentSpeed", &T::tangentSpeed,
    "userMaterialId", &T::userMaterialId,
    "customColor", &T::customColor
  );
};

template <>
struct glz::meta<b2Filter> {
  using T = b2Filter;
  static constexpr auto value = object(
    "categoryBits", &T::categoryBits,
    "maskBits", &T::maskBits,
    "groupIndex", &T::groupIndex
  );
};

template <>
struct glz::meta<b2ShapeDef> {
  using T = b2ShapeDef;
  static constexpr auto value = object(
    "material", &T::material,
    "density", &T::density,
    "filter", &T::filter,
    "isSensor", &T::isSensor,
    "enableSensorEvents", &T::enableSensorEvents,
    "enableContactEvents", &T::enableContactEvents,
    "enableHitEvents", &T::enableHitEvents,
    "enablePreSolveEvents", &T::enablePreSolveEvents,
    "invokeContactCreation", &T::invokeContactCreation,
    "updateBodyMass", &T::updateBodyMass
  );
};

template <>
struct glz::meta<b2Circle> {
  using T = b2Circle;
  static constexpr auto value = object(
    "radius", &T::radius
  );
};

template <>
struct glz::meta<b2Vec2> {
  using T = b2Vec2;
  static constexpr auto value = object(
    "x", &T::x,
    "y", &T::y
  );
};

template <>
struct glz::meta<b2Polygon> {
  using T = b2Polygon;
  static constexpr auto value = object(
    "vertices", &T::vertices,
    "normals", &T::normals,
    "centroid", &T::centroid,
    "radius", &T::radius,
    "count", &T::count
  );
};

template<>
struct glz::meta<AddShapeComponent::ShapeVariant> {
  static constexpr std::string_view tag = "type";
  static constexpr auto ids = std::array{
      "circle",
      "polygon"
  };
};

template <>
struct glz::meta<AddShapeComponent> {
  using T = AddShapeComponent;
  static constexpr auto value = object(
    "def", &T::def,
    "shape", &T::shape
  );
};