#pragma once

#include "../../common/common.hpp"
#include "../../core/transform.hpp"

RAMPAGE_START

enum PhysicsCategories { Friendly = 0x01, Enemy = 0x02, Static = 0x04, All = 0xFFFF };

struct BodyComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component) {
    auto bodyComponentBuilder = builder.initRoot<Schema::BodyComponent>();
    auto bodyBuilder = bodyComponentBuilder.initBody();
    auto self = component.cast<BodyComponent>();

    // Only serialize if body is valid
    if (!b2Body_IsValid(self->id)) {
      bodyBuilder.setBodyType(Schema::BodyType::INVALID);
      bodyBuilder.getLinearVelocity().setX(0);
      bodyBuilder.getLinearVelocity().setY(0);
      return;
    }
    
    // Serialize body properties using Box2D C API
    bodyBuilder.setBodyType(static_cast<Schema::BodyType>(b2Body_GetType(self->id)));
    
    b2Vec2 linearVel = b2Body_GetLinearVelocity(self->id);
    bodyBuilder.getLinearVelocity().setX(linearVel.x);
    bodyBuilder.getLinearVelocity().setY(linearVel.y);
    
    bodyBuilder.setAngularVelocity(b2Body_GetAngularVelocity(self->id));
    bodyBuilder.setLinearDamping(b2Body_GetLinearDamping(self->id));
    bodyBuilder.setAngularDamping(b2Body_GetAngularDamping(self->id));
    bodyBuilder.setGravityScale(b2Body_GetGravityScale(self->id));
    bodyBuilder.setSleepThreshold(b2Body_GetSleepThreshold(self->id));
    bodyBuilder.setEnableSleep(b2Body_IsSleepEnabled(self->id));
    bodyBuilder.setIsAwake(b2Body_IsAwake(self->id));
    bodyBuilder.setIsBullet(b2Body_IsBullet(self->id));
    bodyBuilder.setIsEnabled(b2Body_IsEnabled(self->id));
    bodyBuilder.setAllowFastRotation(false);

    // Serialize shapes - using array-based access
    int shapeCount = b2Body_GetShapeCount(self->id);
    auto shapesBuilder = bodyBuilder.initShapes(shapeCount);

    std::vector<b2ShapeId> shapeIds(shapeCount);
    b2Body_GetShapes(self->id, shapeIds.data(), shapeCount);

    for (int i = 0; i < shapeCount; ++i) {
      b2ShapeId shapeId = shapeIds[i];
      auto shapeBuilder = shapesBuilder[i];
      auto defBuilder = shapeBuilder.initDef();
      
      b2Filter filter = b2Shape_GetFilter(shapeId);
      
      // Filter
      auto filterBuilder = defBuilder.initFilter();
      filterBuilder.setCategoryBits(filter.categoryBits);
      filterBuilder.setMaskBits(filter.maskBits);
      filterBuilder.setGroupIndex(filter.groupIndex);

      // Shape properties
      defBuilder.setDensity(b2Shape_GetDensity(shapeId));
      defBuilder.setIsSensor(b2Shape_IsSensor(shapeId));
      defBuilder.setEnableContactEvents(true);

      // Shape variant - determine by type
      b2ShapeType shapeType = b2Shape_GetType(shapeId);
      if (shapeType == b2_circleShape) {
        b2Circle circle = b2Shape_GetCircle(shapeId);
        auto circleBuilder = shapeBuilder.initCircle();
        circleBuilder.setRadius(circle.radius);
      } else if (shapeType == b2_polygonShape) {
        b2Polygon poly = b2Shape_GetPolygon(shapeId);
        auto polyBuilder = shapeBuilder.initPolygon();
        auto verts = polyBuilder.initVertices(poly.count);
        for (int j = 0; j < poly.count; ++j) {
          verts[j].setX(poly.vertices[j].x);
          verts[j].setY(poly.vertices[j].y);
        }
        polyBuilder.setCount(poly.count);
      }
    }
  }

  // Helper method to fully reconstruct body and shapes when world is available
  static void deserialize(capnp::MessageReader& reader, Ref component) {
    auto bodyComponentReader = reader.getRoot<Schema::BodyComponent>();
    auto bodyReader = bodyComponentReader.getBody();
    auto self = component.cast<BodyComponent>();
    b2WorldId world = component.getWorld()->getContext<b2WorldId>();

    if(bodyReader.getBodyType() == Schema::BodyType::INVALID) {
      self->id = b2_nullBodyId;
      return;
    }

    // Create body with deserialized properties
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = static_cast<b2BodyType>(bodyReader.getBodyType());
    
    auto linearVelReader = bodyReader.getLinearVelocity();
    bodyDef.linearVelocity = b2Vec2{linearVelReader.getX(), linearVelReader.getY()};
    
    bodyDef.angularVelocity = bodyReader.getAngularVelocity();
    bodyDef.linearDamping = bodyReader.getLinearDamping();
    bodyDef.angularDamping = bodyReader.getAngularDamping();
    bodyDef.gravityScale = bodyReader.getGravityScale();
    bodyDef.sleepThreshold = bodyReader.getSleepThreshold();
    bodyDef.enableSleep = bodyReader.getEnableSleep();
    bodyDef.isAwake = bodyReader.getIsAwake();
    bodyDef.isBullet = bodyReader.getIsBullet();
    bodyDef.isEnabled = bodyReader.getIsEnabled();
    bodyDef.allowFastRotation = bodyReader.getAllowFastRotation();

    // Try to set body position from Transform if available
    // This ensures the body is at the correct position before shapes are added
    auto entity = component.getEntity();
    if (entity.has<TransformComponent>()) {
      auto transform = entity.get<TransformComponent>();
      bodyDef.position = transform->pos;
      bodyDef.rotation = transform->rot;
    }

    // Create the body
    self->id = b2CreateBody(world, &bodyDef);
    
    // Recreate shapes
    auto shapesReader = bodyReader.getShapes();
    for (auto shapeReader : shapesReader) {
      auto defReader = shapeReader.getDef();
      b2ShapeDef shapeDef = b2DefaultShapeDef();

      shapeDef.density = defReader.getDensity();
      shapeDef.isSensor = defReader.getIsSensor();
      shapeDef.enableContactEvents = defReader.getEnableContactEvents();

      auto filterReader = defReader.getFilter();
      shapeDef.filter.categoryBits = filterReader.getCategoryBits();
      shapeDef.filter.maskBits = filterReader.getMaskBits();
      shapeDef.filter.groupIndex = filterReader.getGroupIndex();

      // Recreate shape based on type
      if (shapeReader.which() == Schema::Shape::Which::CIRCLE) {
        b2Circle circle;
        circle.center = Vec2(0);
        circle.radius = shapeReader.getCircle().getRadius();
        b2CreateCircleShape(self->id, &shapeDef, &circle);
      } else if (shapeReader.which() == Schema::Shape::Which::POLYGON) {
        auto polyReader = shapeReader.getPolygon();
        int count = polyReader.getCount();
        
        // Create polygon from vertices
        auto vertsReader = polyReader.getVertices();
        b2Vec2 vertices[B2_MAX_POLYGON_VERTICES];
        for (int i = 0; i < count; ++i)
          vertices[i] = b2Vec2{vertsReader[i].getX(), vertsReader[i].getY()};

        b2Hull hull = b2ComputeHull(vertices, count);
        b2Polygon poly = b2MakePolygon(&hull, 0.0f); 
        b2CreatePolygonShape(self->id, &shapeDef, &poly);
      }
    }
  }

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
  AddBodyComponent() {
    dynamic_cast<b2BodyDef&>(*this) = b2DefaultBodyDef();
  }

  AddBodyComponent(glz::make_reflectable) {
    dynamic_cast<b2BodyDef&>(*this) = b2DefaultBodyDef();
  }
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
             &T::enableSleep, "isAwake", &T::isAwake, "isBullet", &T::isBullet, "isEnabled", &T::isEnabled,
             "allowFastRotation", &T::allowFastRotation);
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
