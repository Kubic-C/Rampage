#pragma once

#include "../../common/common.hpp"
#include "../../core/transform.hpp"

RAMPAGE_START

enum class UserDataType: u8 { 
  Entity,
  Tile
};

struct EntityBox2dUserData {
  EntityBox2dUserData() = default;
  EntityBox2dUserData(UserDataType type)
    : type(type) {}

  static EntityBox2dUserData* create(EntityId entity) {
    EntityBox2dUserData* data = new EntityBox2dUserData();
    data->entity = entity;
    return data;
  }

  UserDataType type = UserDataType::Entity; 
  EntityId entity = NullEntityId;
};

inline EntityBox2dUserData* getEntityBox2dUserData(b2ShapeId shape) {
  return static_cast<EntityBox2dUserData*>(b2Shape_GetUserData(shape));
}

inline EntityBox2dUserData* getEntityBox2dUserData(b2BodyId body) {
  return static_cast<EntityBox2dUserData*>(b2Body_GetUserData(body));
}

enum PhysicsCategories { Friendly = 0x01, Enemy = 0x02, Static = 0x04, Item = 0x08, All = 0xFFFF };

struct BodyComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component);
  static void deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component);
  static void fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& compJson);
  static void copy(Ref src, Ref dst);

private:
  static void addShapeFromJson(b2BodyId bodyId, const JSchema::ShapesItem& shapeJson);
  static uint64_t convertCategoryMaskToBits(const std::vector<std::string>& masks);
  
public:

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
      EntityBox2dUserData* data = static_cast<EntityBox2dUserData*>(b2Body_GetUserData(id));
      if(data && data->type == UserDataType::Entity)
        delete data;

      size_t shapeCount = b2Body_GetShapeCount(id);
      std::vector<b2ShapeId> shapeIds(shapeCount);
      shapeCount = b2Body_GetShapes(id, shapeIds.data(), shapeCount);
      shapeIds.resize(shapeCount);
      for(b2ShapeId shapeId : shapeIds) {
        data = static_cast<EntityBox2dUserData*>(b2Shape_GetUserData(shapeId));
        if (data && data->type == UserDataType::Entity)
          delete data;
      }

      b2DestroyBody(id);
    }
  }

  operator b2BodyId() const {
    return id;
  }
};

RAMPAGE_END
