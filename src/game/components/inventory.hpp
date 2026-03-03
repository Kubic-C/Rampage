#pragma once

#include "../../common/common.hpp"

RAMPAGE_START

/**
 * ItemComponent - Attached to item entities (stackable or unique)
 * For stackable items: single entity shared across multiple stacks
 * For unique items: each has its own entity with this component
 */
struct ItemComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component);
  static void deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component);
  static void fromJson(Ref component, AssetLoader loader, const json& compJson);

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
struct ItemStack {
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
  static void fromJson(Ref component, AssetLoader loader, const json& compJson);

  std::vector<ItemStack> items;  // Linear array of slots
  u16 cols = 3;
  u16 rows = 3;

  // Utility to convert 2D grid position to linear index
  size_t getSlotIndex(u16 x, u16 y) const {
    return y * cols + x;
  }

  // Get slot at 2D position
  ItemStack& getSlot(u16 x, u16 y) {
    return items[getSlotIndex(x, y)];
  }

  const ItemStack& getSlot(u16 x, u16 y) const {
    return items[getSlotIndex(x, y)];
  }
};

struct InventoryViewComponent {
  EntityId inventoryEntityId;

  std::string name = "Inventory";
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

  void update(IWorldPtr world, tgui::Gui& gui);

private:
  tgui::ChildWindow::Ptr window;
  std::vector<tgui::Panel::Ptr> slotBackgrounds;
  std::vector<tgui::Picture::Ptr> slotPictures;
  std::vector<tgui::Label::Ptr> slotLabels;

  // Track grid size changes to rebuild UI when inventory dimensions change
  u16 cachedRows = 0;
  u16 cachedCols = 0;

  // Drag and drop state (static = global drag state, only one drag at a time)
  static bool isDragging;
  static glm::u16vec2 dragStartSlot;
  static InventoryViewComponent* dragSourceView;  // Pointer to the view we're dragging from

  // Slot click/drag callbacks
  void onSlotMouseDown(IWorldPtr world, glm::u16vec2 slotPos);
  void onSlotMouseUp(IWorldPtr world, glm::u16vec2 slotPos);
  
  // Helper to perform the actual item move
  void performItemDrop(IWorldPtr world, InventoryViewComponent* sourceView, glm::u16vec2 sourceSlot, 
                       glm::u16vec2 targetSlot);
};

RAMPAGE_END