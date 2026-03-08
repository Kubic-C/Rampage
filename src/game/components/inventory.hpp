#pragma once

#include "../../common/common.hpp"
#include "items.hpp"

RAMPAGE_START

struct ItemRecieverComponent {

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
  InventoryViewComponent() = default;
  ~InventoryViewComponent();

  InventoryViewComponent& operator=(const InventoryViewComponent& other) noexcept {
    name = other.name;
    pos = other.pos;
    isVisible = other.isVisible;
    isInteractable = other.isInteractable;
    padding = other.padding;
    slotSize = other.slotSize;
    rounding = other.rounding;
    windowBackgroundColor = other.windowBackgroundColor;
    borderColor = other.borderColor;
    emptySlotColor = other.emptySlotColor;
    textColor = other.textColor;
    hoverSlotColor = other.hoverSlotColor;
    dragHoverSlotColor = other.dragHoverSlotColor;

    prevVisualChecksum = other.prevVisualChecksum;
    inventoryEntityId = other.inventoryEntityId;

    return *this;
  };

  // Move constructor
  InventoryViewComponent(InventoryViewComponent&& other) noexcept
    : name(std::move(other.name)),
      window(std::move(other.window)),
      slotBackgrounds(std::move(other.slotBackgrounds)),
      slotPictures(std::move(other.slotPictures)),
      slotLabels(std::move(other.slotLabels)),
      tooltipPanel(std::move(other.tooltipPanel)),
      tooltipIcon(std::move(other.tooltipIcon)),
      tooltipName(std::move(other.tooltipName)),
      tooltipDescription(std::move(other.tooltipDescription)),
      tooltipUnique(std::move(other.tooltipUnique)) {
    pos = other.pos;
    isVisible = other.isVisible;
    isInteractable = other.isInteractable;
    padding = other.padding;
    slotSize = other.slotSize;
    rounding = other.rounding;
    windowBackgroundColor = other.windowBackgroundColor;
    borderColor = other.borderColor;
    emptySlotColor = other.emptySlotColor;
    textColor = other.textColor;
    hoverSlotColor = other.hoverSlotColor;
    dragHoverSlotColor = other.dragHoverSlotColor;

    prevVisualChecksum = other.prevVisualChecksum;
    inventoryEntityId = other.inventoryEntityId;
  }

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

  static void update(EntityPtr inventoryEntity, tgui::Gui& gui);

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
  u32 prevVisualChecksum = 0;
  EntityId inventoryEntityId = 0; // The entity this view is representing (for drag/drop)

  // Drag and drop state (static = global drag state, only one drag at a time)
  static bool isDragging;
  static glm::u16vec2 dragStartSlot;
  static EntityId dragSourceEntity;  // Entity ID of the view we're dragging from
  static bool wasMouseButtonPressedLastFrame;  // Track previous frame mouse state for outside-release detection

  // Active placement mode state (double-click a slot to paint tiles in the world)
  static bool isPlacementMode;
  static glm::u16vec2 activePlacementSlot;
  static EntityId activePlacementEntity;

  // Checksum is based off visual config and inventory size - if it changes, we need to rebuild the UI
  static u32 calculateVisualChecksum(RefT<InventoryViewComponent> self, u32 sizeX, u32 sizeY);
  static bool hasVisualConfigChanged(RefT<InventoryViewComponent> self, u32 sizeX, u32 sizeY, u32 previousChecksum);

  // Slot click/drag callbacks
  static void onSlotMouseDown(EntityPtr inventoryEntity, glm::u16vec2 slotPos);
  static void onSlotMouseUp(EntityPtr inventoryEntity, glm::u16vec2 slotPos);
  static void onSlotDoubleClick(EntityPtr inventoryEntity, glm::u16vec2 slotPos);
  
  // Tooltip display helpers
  static void showTooltip(EntityPtr inventoryEntity, size_t slotIndex);
  static void hideTooltip(RefT<InventoryViewComponent> self);
  
  // Helper to perform the actual item move
  static void performItemDrop(EntityPtr inventoryEntity, EntityId sourceEntity, glm::u16vec2 sourceSlot, 
                       glm::u16vec2 targetSlot);
  
  // Helper to check if a world position is within the inventory window bounds
  static bool isPointInWindowBounds(RefT<InventoryViewComponent> self, const glm::vec2& worldPos);
  
  // Helper to place item to world when released outside inventory with Shift pressed
  static void placeItemToWorld(EntityPtr inventoryEntity);
  
  // Helper to drop item to world when released outside inventory
  static void dropItemToWorld(EntityPtr inventoryEntity);
};

RAMPAGE_END