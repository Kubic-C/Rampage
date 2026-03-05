#pragma once

#include "../../common/common.hpp"

RAMPAGE_START

struct ItemUsePlaceEvent {
  Vec2 placePosition; // The world position where the item is being placed
};

struct ItemUseConsumeEvent {};

/**
 * ItemUseComponent - Links items to gameplay interactions
 * Attached to entities that can use/activate items
 * Stores references to item entities and their usage parameters
 */
struct ItemUseComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component);
  static void deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component);
  static void fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& compJson);

  EntityId entityId = 0;            // Reference to the entity to use/summon/place/...
  float effectValue = 0.0f;         // Magnitude of the effect
  float effectRadius = 0.0f;        // For area-of-effect abilities
  float cooldown = 0.0f;            // Cooldown between uses (in seconds)
  float remainingCooldown = 0.0f;   // Time left on current cooldown
  int maxCharges = 1;               // Max uses before recharge (for limited-use items)
  int currentCharges = 1;           // Current charges remaining
  bool isActive = true;             // Whether the item use is currently active
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

/**
 * InventoryComponent - Attached to inventory entities
 * Container for slots, manages item organization
 */
struct InventoryComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component);
  static void deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component);
  static void fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& compJson);

  std::vector<ItemStackComponent> items;  // Linear array of slots
  u16 cols = 3;
  u16 rows = 3;

  // Utility to convert 2D grid position to linear index
  size_t getSlotIndex(u16 x, u16 y) const {
    return y * cols + x;
  }

  // Get slot at 2D position
  ItemStackComponent& getSlot(u16 x, u16 y) {
    return items[getSlotIndex(x, y)];
  }

  const ItemStackComponent& getSlot(u16 x, u16 y) const {
    return items[getSlotIndex(x, y)];
  }
};

struct InventoryViewComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component);
  static void deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component);
  static void fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& compJson);
  ~InventoryViewComponent();

  std::string name = "Inventory";
  Vec2 pos = Vec2(100, 100);
  bool isVisible = false;
  bool isInteractable = true;
  Vec2 padding = Vec2(5, 5);
  float slotSize = 48.0f;
  float rounding = 0.0f;
  tgui::Color windowBackgroundColor = tgui::Color(150, 150, 150, 150);
  tgui::Color borderColor = tgui::Color(120, 120, 120);
  tgui::Color emptySlotColor = tgui::Color::Black;
  tgui::Color textColor = tgui::Color::White;
  tgui::Color hoverSlotColor = tgui::Color(200, 100, 100, 200);
  tgui::Color dragHoverSlotColor = tgui::Color(100, 200, 100, 200);

  void update(EntityPtr inventoryEntity, tgui::Gui& gui);

private:
  tgui::ChildWindow::Ptr window;
  std::vector<tgui::Panel::Ptr> slotBackgrounds;
  std::vector<tgui::Picture::Ptr> slotPictures;
  std::vector<tgui::Label::Ptr> slotLabels;

  // Tooltip panel and widgets
  tgui::Panel::Ptr tooltipPanel;
  tgui::Picture::Ptr tooltipIcon;
  tgui::Label::Ptr tooltipName;
  tgui::Label::Ptr tooltipDescription;
  tgui::Label::Ptr tooltipUnique;

  // Track grid size changes to rebuild UI when inventory dimensions change
  u32 prevVisualChecksum;
  EntityId inventoryEntityId; // The entity this view is representing (for drag/drop)

  // Drag and drop state (static = global drag state, only one drag at a time)
  static bool isDragging;
  static glm::u16vec2 dragStartSlot;
  static InventoryViewComponent* dragSourceView;  // Pointer to the view we're dragging from
  static bool wasMouseButtonPressedLastFrame;  // Track previous frame mouse state for outside-release detection

  // Checksum is based off visual config and inventory size - if it changes, we need to rebuild the UI
  u32 calculateVisualChecksum(u32 sizeX, u32 sizeY) const;
  bool hasVisualConfigChanged(u32 sizeX, u32 sizeY, u32 previousChecksum) const;

  // Slot click/drag callbacks
  void onSlotMouseDown(EntityPtr inventoryEntity, glm::u16vec2 slotPos);
  void onSlotMouseUp(EntityPtr inventoryEntity, glm::u16vec2 slotPos);
  
  // Tooltip display helpers
  void showTooltip(EntityPtr inventoryEntity, size_t slotIndex);
  void hideTooltip();
  
  // Helper to perform the actual item move
  void performItemDrop(EntityPtr inventoryEntity, InventoryViewComponent* sourceView, glm::u16vec2 sourceSlot, 
                       glm::u16vec2 targetSlot);
  
  // Helper to check if a world position is within the inventory window bounds
  bool isPointInWindowBounds(const glm::vec2& worldPos) const;
  
  // Helper to drop item to world when released outside inventory
  void dropItemToWorld(EntityPtr inventoryEntity);
};

RAMPAGE_END