#pragma once

#include "../../common/common.hpp"

RAMPAGE_START


struct ItemDroppedTag : SerializableTag, JsonableTag  {};
struct ItemPlacedTag : SerializableTag, JsonableTag {};

struct ItemPlaceableComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component);
  static void deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component);
  static void fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& compJson);

  EntityId entityId = 0; // The entity that will be placed when this item is used
};

/**
 * ItemComponent - Attached to item entities (stackable or unique)
 * For stackable items: single entity shared across multiple stacks
 * For unique items: each has its own entity with this component
 */
struct ItemComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component);
  static void deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component);
  static void fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& compJson);

  std::string name;
  std::string description;
  u32 maxStackSize = 64;      // Max items per stack (only for non-unique)
  u32 stackCost = 1;          // Weight/cost per item
  bool isUnique = false;      // If true, each instance is a separate entity
  tgui::Texture icon;         // UI icon for rendering

  size_t getTotalStackCost(size_t count) const {
    return stackCost * count;
  }
};

/**
 * ItemStack - Represents one slot in an inventory
 * References an item entity and holds a count
 * If item has ItemUnique flag, count is always 1
 */
struct ItemStackComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component);
  static void deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component);
  static void fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& compJson);

  u32 count = 0;        // How many of this item in this slot
  EntityId itemId = 0;  // Entity ID of the item type (or unique instance)
};

struct ItemRecipeComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component);
  static void deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component);
  static void fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& compJson);

  EntityId resultItemId = 0; // Entity ID of the resulting item
  Set<EntityId> ingredientItemIds; // Set of item entity IDs required as ingredients
  u32 craftingTime = 1; // In ticks
  u32 amountProduced = 1; // The amount of produced items
};

enum class PortDirection : u8 { In, Out };
enum class PortDistribution : u8 { RoundRobin, Priority, First };

struct PortComponent {
  PortDirection direction = PortDirection::In;
  PortDistribution distribution = PortDistribution::RoundRobin;
  glm::ivec2 tileOffset = {0, 0};     // offset from factory origin tile
  u8 side = 0;                          // 0=N,1=E,2=S,3=W
  std::vector<EntityId> connections;    // connected conveyor IDs
  u32 currentConnectionIndex = 0;       // for round-robin
};

struct ConveyorItem {
  EntityId itemId = 0;
  u32 count = 0;
  float progress = 0.0f;  // 0.0 = at source, 1.0 = arrived
};

struct ConveyorComponent {
  EntityId sourcePortId = 0;
  EntityId destPortId = 0;
  float speed = 1.0f;              // progress per second (precomputed from distance + belt speed)
  float tileDistance = 1.0f;       // cached distance in tiles
  std::vector<ConveyorItem> items; // items in transit
  u8 maxInFlight = 4;             // capacity cap
};

struct ConveyorPartComponent {
  EntityId conveyorId = 0;
};

RAMPAGE_END
