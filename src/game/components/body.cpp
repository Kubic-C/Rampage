#include "body.hpp"

RAMPAGE_START

void BodyComponent::serialize(capnp::MessageBuilder& builder, Ref component) {
  auto bodyComponentBuilder = builder.initRoot<Schema::BodyComponent>();
  auto bodyBuilder = bodyComponentBuilder.initBody();
  auto self = component.cast<BodyComponent>();

  // Only serialize if body is valid
  if (!b2Body_IsValid(self->id)) {
    bodyBuilder.setBodyType(Schema::BodyType::INVALID_BODY);
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
void BodyComponent::deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component) {
  auto bodyComponentReader = reader.getRoot<Schema::BodyComponent>();
  auto bodyReader = bodyComponentReader.getBody();
  auto self = component.cast<BodyComponent>();
  b2WorldId world = component.getWorld()->getContext<b2WorldId>();

  if(bodyReader.getBodyType() == Schema::BodyType::INVALID_BODY) {
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

b2BodyType convertJSONToBodyType(const std::string& typeStr) {
  static const std::unordered_map<std::string, b2BodyType> bodyTypeMapping = {
      {"dynamic", b2_dynamicBody}, {"static", b2_staticBody}, {"kinematic", b2_kinematicBody}};

  return bodyTypeMapping.at(typeStr);
}
  
void BodyComponent::fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& jsonValue) {
  EntityPtr entity = component.getEntity();
  b2WorldId world = component.getWorld()->getContext<b2WorldId>();
  auto body = component.cast<BodyComponent>();
  auto compJson = jsonValue.as<JSchema::BodyComponent>();

  // Create body definition
  b2BodyDef bodyDef = b2DefaultBodyDef();
  bodyDef.isEnabled = false; // fromJson means this component is being created from an asset, so we start disabled. The system that creates the entity from the asset can then enable it when ready.
  
  // Explicitly zero velocity for freshly created bodies from assets
  bodyDef.linearVelocity = b2Vec2{0.0f, 0.0f};
  bodyDef.angularVelocity = 0.0f;
  
  // Set body type (dynamic=1, kinematic=2, static=0)
  if (compJson->hasBodyType()) {
    bodyDef.type = convertJSONToBodyType(compJson->getBodyType());
  } else {
    bodyDef.type = b2_dynamicBody;
  }

  // Set body properties
  if (compJson->hasLinearDamping())
    bodyDef.linearDamping = compJson->getLinearDamping();
  if (compJson->hasAngularDamping())
    bodyDef.angularDamping = compJson->getAngularDamping();
  if (compJson->hasGravityScale())
    bodyDef.gravityScale = compJson->getGravityScale();
  if (compJson->hasIsBullet())
    bodyDef.isBullet = compJson->getIsBullet();
  if (compJson->hasEnableSleep())
    bodyDef.enableSleep = compJson->getEnableSleep();

  // Position from transform if available
  if (entity.has<TransformComponent>()) {
    auto transform = entity.get<TransformComponent>();
    bodyDef.position = transform->pos;
    bodyDef.rotation = transform->rot;
  }

  // Create the body
  body->id = b2CreateBody(world, &bodyDef);

  // Add shapes if provided
  if (compJson->hasShapes()) {
    for (const auto& shapeItem : compJson->getShapes()) {
      addShapeFromJson(body->id, shapeItem);
    }
  }
}

void BodyComponent::addShapeFromJson(b2BodyId bodyId, const JSchema::ShapesItem& shapeJson) {
  b2ShapeDef shapeDef = b2DefaultShapeDef();

  // Parse shape definition
  if (shapeJson.hasEnableContactEvents())
    shapeDef.enableContactEvents = shapeJson.getEnableContactEvents();
  if (shapeJson.hasEnableSensorEvents())
    shapeDef.enableSensorEvents = shapeJson.getEnableSensorEvents();
  if (shapeJson.hasDensity())
    shapeDef.density = shapeJson.getDensity();
  if (shapeJson.hasFriction())
    shapeDef.material.friction = shapeJson.getFriction();
  if (shapeJson.hasRestitution())
    shapeDef.material.restitution = shapeJson.getRestitution();
  if (shapeJson.hasIsSensor())
    shapeDef.isSensor = shapeJson.getIsSensor();

  // Convert and apply category/collision masks
  if (shapeJson.hasCategoryMask()) {
    shapeDef.filter.categoryBits = convertCategoryMaskToBits(shapeJson.getCategoryMask());
  }
  if (shapeJson.hasCollisionMask()) {
    shapeDef.filter.maskBits = convertCategoryMaskToBits(shapeJson.getCollisionMask());
  }

  // Parse and create shape variant
  if (shapeJson.hasShape()) {
    const auto& shapeDef2 = shapeJson.getShape();
    const std::string& shapeType = shapeDef2.getShapeType();

    if (shapeType == "Circle") {
      b2Circle circle{0};
      if (shapeDef2.hasRadius())
        circle.radius = shapeDef2.getRadius();
      b2CreateCircleShape(bodyId, &shapeDef, &circle);
    } else if (shapeType == "Polygon") {
      b2Polygon polygon{0};
      if (shapeDef2.hasVertices()) {
        const auto& vertices = shapeDef2.getVertices();
        if (vertices.size() <= 8) {
          size_t size = std::min((size_t)B2_MAX_POLYGON_VERTICES, vertices.size());
          b2Vec2 verts[B2_MAX_POLYGON_VERTICES];
          for (size_t i = 0; i < size; ++i)
            verts[i] = b2Vec2{(float)vertices[i].getX(), (float)vertices[i].getY()};

          b2Hull hull = b2ComputeHull(verts, size);
          polygon = b2MakePolygon(&hull, 0.0f); 
        }
      }
      b2CreatePolygonShape(bodyId, &shapeDef, &polygon);
    }
  }
}

void BodyComponent::copy(Ref src, Ref dst) {
  const b2BodyId& srcId = src.cast<BodyComponent>()->id;  
  b2BodyId& dstId = dst.cast<BodyComponent>()->id;

  if (!b2Body_IsValid(srcId)) {
    dstId = b2_nullBodyId;
    return;
  }

  // Get world from the destination component
  auto dstComponent = dst.cast<BodyComponent>();
  b2WorldId world = dst.getWorld()->getContext<b2WorldId>();

  // Destroy existing destination body if valid
  if (b2Body_IsValid(dstId)) {
    b2DestroyBody(dstId);
  }

  // Create body definition from source - copy all properties
  b2BodyDef bodyDef = b2DefaultBodyDef();
  bodyDef.type = b2Body_GetType(srcId);
  bodyDef.position = b2Body_GetPosition(srcId);
  bodyDef.rotation = b2Body_GetRotation(srcId);
  bodyDef.linearVelocity = b2Body_GetLinearVelocity(srcId);
  bodyDef.angularVelocity = b2Body_GetAngularVelocity(srcId);
  bodyDef.linearDamping = b2Body_GetLinearDamping(srcId);
  bodyDef.angularDamping = b2Body_GetAngularDamping(srcId);
  bodyDef.gravityScale = b2Body_GetGravityScale(srcId);
  bodyDef.sleepThreshold = b2Body_GetSleepThreshold(srcId);
  bodyDef.enableSleep = b2Body_IsSleepEnabled(srcId);
  // bodyDef.isAwake = b2Body_IsAwake(srcId);
  bodyDef.isBullet = b2Body_IsBullet(srcId);
  // bodyDef.isEnabled = b2Body_IsEnabled(srcId);
  bodyDef.allowFastRotation = false;

  // Create the new body
  dstId = b2CreateBody(world, &bodyDef);

  // Clone all shapes from source to destination
  int shapeCount = b2Body_GetShapeCount(srcId);
  std::vector<b2ShapeId> shapeIds(shapeCount);
  b2Body_GetShapes(srcId, shapeIds.data(), shapeCount);

  for (int i = 0; i < shapeCount; ++i) {
    b2ShapeId srcShapeId = shapeIds[i];
    b2ShapeDef shapeDef = b2DefaultShapeDef();

    // Copy shape properties
    b2Filter filter = b2Shape_GetFilter(srcShapeId);
    shapeDef.filter = filter;
    shapeDef.density = b2Shape_GetDensity(srcShapeId);
    shapeDef.isSensor = b2Shape_IsSensor(srcShapeId);
    shapeDef.enableContactEvents = true;

    // Clone shape based on type
    b2ShapeType shapeType = b2Shape_GetType(srcShapeId);
    if (shapeType == b2_circleShape) {
      b2Circle circle = b2Shape_GetCircle(srcShapeId);
      b2CreateCircleShape(dstId, &shapeDef, &circle);
    } else if (shapeType == b2_polygonShape) {
      b2Polygon poly = b2Shape_GetPolygon(srcShapeId);
      b2CreatePolygonShape(dstId, &shapeDef, &poly);
    }
  }
}

uint64_t BodyComponent::convertCategoryMaskToBits(const std::vector<std::string>& masks) {
  static const std::unordered_map<std::string, uint64_t> categoryMapping{
      {"Friendly", Friendly}, {"Enemy", Enemy}, {"Static", Static}, {"All", All}};

  uint64_t bitmask = 0;
  for (const auto& catStr : masks) {
    auto it = categoryMapping.find(catStr);
    if (it != categoryMapping.end()) {
      bitmask |= it->second;
    }
  }
  return bitmask;
}

RAMPAGE_END