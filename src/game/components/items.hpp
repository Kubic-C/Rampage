#pragma once

#include "tilemap.hpp"

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

enum class PortDistribution : u8 { RoundRobin, Priority, First };

struct ConveyorComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component);
  static void deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component);

  struct InventoryDistance {
    EntityId inventoryId;
    float virtualDistance; // Distance from the port to the inventory (used for timing item transfers)
  };

  struct ItemInTransit {
    ItemStackComponent stack;
    u32 invIndex;
    float curVirtualDistance = 0.0f; 
  };

  std::vector<ItemInTransit> itemsInTransit; // Items currently being transferred through this port
  size_t maxItemsInTransit = 5; // Maximum number of items that can be in transit at once
  std::vector<InventoryDistance> inventories;
};

struct PortComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component);
  static void deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component);
  static void fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& compJson);

  EntityId importingInventory = 0;
  PortDistribution distribution = PortDistribution::RoundRobin;
  size_t exportingIndex = 0; // For priority distribution, use in tandem with exportingInventories.
  u32 ticksPerUpdate = 20; // Cooldown between updates in ticks
  u32 tickCounter = 0; // Internal counter to track ticks for update cooldown
  Set<EntityId> filter; // Optional filter for allowed item types (empty means allow all)
  bool isOn = true;

  EntityId exportingConveyor = 0; // The conveyor entity this port is connected to (if any)
};

struct ConveyorPartComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component);
  static void deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component);
  static void fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& compJson);

  EntityId conveyorId = 0;
  std::vector<TileDirection> inputDirections;
  float virtualDistanceIncrease = 0.5f;
  TileDirection cachedRotation = TileDirection::Right; // tracks last-known tile rotation for revalidation
};

struct FactoryComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component);
  static void deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component);
  static void fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& compJson);

  Set<EntityId> recipeIds; // Set of ItemRecipeComponent entity IDs that this factory can produce
};

RAMPAGE_END
