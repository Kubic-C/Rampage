#pragma once

#include "../../common/common.hpp"
#include "../../core/transform.hpp"

RAMPAGE_START

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
      b2DestroyBody(id);
    }
  }

  operator b2BodyId() const {
    return id;
  }
};

RAMPAGE_END
