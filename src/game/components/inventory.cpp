#include "inventory.hpp"
#include "../systems/inventory.hpp"
#include "../../event/module.hpp"
#include "../../render/module.hpp"
#include <SDL3/SDL.h>

RAMPAGE_START

// Static member definitions for global drag state
bool InventoryViewComponent::isDragging = false;
glm::u16vec2 InventoryViewComponent::dragStartSlot = {0, 0};
EntityId InventoryViewComponent::dragSourceEntity = 0;
bool InventoryViewComponent::wasMouseButtonPressedLastFrame = false;
bool InventoryViewComponent::isPlacementMode = false;
glm::u16vec2 InventoryViewComponent::activePlacementSlot = {0, 0};
EntityId InventoryViewComponent::activePlacementEntity = 0;

void InventoryComponent::serialize(capnp::MessageBuilder& builder, Ref component) {
  auto invBuilder = builder.initRoot<Schema::InventoryComponent>();
  auto self = component.cast<InventoryComponent>();

  invBuilder.setCols(self->cols);
  invBuilder.setRows(self->rows);

  // Serialize items list
  auto itemsBuilder = invBuilder.initItems((u32)self->items.size());
  for (size_t i = 0; i < self->items.size(); ++i) {
    itemsBuilder[(u32)i].setCount(self->items[i].count);
    itemsBuilder[(u32)i].setItemId(self->items[i].itemId);
  }
}

void InventoryComponent::deserialize(capnp::MessageReader& reader, const IdMapper& idMapper, Ref component) {
  auto invReader = reader.getRoot<Schema::InventoryComponent>();
  auto self = component.cast<InventoryComponent>();

  self->cols = invReader.getCols();
  self->rows = invReader.getRows();

  // Deserialize items list and remap entity IDs
  self->items.clear();
  const auto itemsReader = invReader.getItems();
  for (const auto& itemReader : itemsReader) {
    ItemStackComponent stack;
    stack.count = itemReader.getCount();
    stack.itemId = idMapper.resolve(itemReader.getItemId());
    self->items.push_back(stack);
  }
}

void InventoryComponent::fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& jsonValue) {
  auto self = component.cast<InventoryComponent>();
  auto compJson = jsonValue.as<JSchema::InventoryComponent>();

  if (compJson->hasCols())
    self->cols = compJson->getCols();

  if (compJson->hasRows())
    self->rows = compJson->getRows();

  // Initialize slots
  self->items.clear();
  self->items.resize(self->cols * self->rows); // Initialize slots

  // Optionally load initial items from JSON (for asset definitions)
  if (compJson->hasItems()) {
    const auto& itemsJson = compJson->getItems();
    size_t slotIndex = 0;

    for (const auto& itemJson : itemsJson) {
      if (slotIndex >= self->items.size()) break;

      if (itemJson.hasCount())
        self->items[slotIndex].count = itemJson.getCount();

      if (itemJson.hasItemEntityName()) {
        // Resolve entity by name from asset loader
        self->items[slotIndex].itemId = loader.getAsset(itemJson.getItemEntityName());
      }

      slotIndex++;
    }
  }
}

glm::vec4 getContrastingColor(const glm::vec4& color) {
  float r = color.r;
  float g = color.g;
  float b = color.b;
  
  // Check if color is grey (all RGB values are similar)
  float maxDiff = std::max({r, g, b}) - std::min({r, g, b});
  bool isGrey = maxDiff < 30;  // Threshold for "grey-like"
  
  if (isGrey) {
      // For grey, use brightness to pick white or black
      float brightness = (0.299f * r + 0.587f * g + 0.114f * b);
      return brightness < 0.5f ? gui::WHITE : gui::BLACK;
  } else {
      // For colored backgrounds, invert
      return glm::vec4(1.0f - r / 255.0f, 
                       1.0f - g / 255.0f, 
                       1.0f - b / 255.0f,
                       color.a);
  }
}

// Helper functions to convert color to/from packed UInt32
inline u32 packColor(const glm::vec4& color) {
  return (static_cast<u32>(color.r * 255.0f) << 24) |
         (static_cast<u32>(color.g * 255.0f) << 16) |
         (static_cast<u32>(color.b * 255.0f) << 8) |
         static_cast<u32>(color.a * 255.0f);
}

inline glm::vec4 unpackColor(u32 packed) {
  return glm::vec4(
    ((packed >> 24) & 0xFF) / 255.0f,
    ((packed >> 16) & 0xFF) / 255.0f,
    ((packed >> 8) & 0xFF) / 255.0f,
    (packed & 0xFF) / 255.0f
  );
}

void InventoryViewComponent::serialize(capnp::MessageBuilder& builder, Ref component) {
  auto viewBuilder = builder.initRoot<Schema::InventoryViewComponent>();
  auto self = component.cast<InventoryViewComponent>();

  viewBuilder.setInventoryEntityId(self->inventoryEntityId);
  viewBuilder.setName(self->name);

  auto posBuilder = viewBuilder.initPos();
  posBuilder.setX(self->pos.x);
  posBuilder.setY(self->pos.y);

  viewBuilder.setIsVisible(self->isVisible);
  viewBuilder.setIsInteractable(self->isInteractable);

  auto paddingBuilder = viewBuilder.initPadding();
  paddingBuilder.setX(self->padding.x);
  paddingBuilder.setY(self->padding.y);

  viewBuilder.setSlotSize(self->slotSize);
  viewBuilder.setRounding(self->rounding);

  viewBuilder.setWindowBackgroundColor(packColor(self->windowBackgroundColor));
  viewBuilder.setBorderColor(packColor(self->borderColor));
  viewBuilder.setEmptySlotColor(packColor(self->emptySlotColor));
  viewBuilder.setTextColor(packColor(self->textColor));
  viewBuilder.setHoverSlotColor(packColor(self->hoverSlotColor));
  viewBuilder.setDragHoverSlotColor(packColor(self->dragHoverSlotColor));
}

void InventoryViewComponent::deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component) {
  auto viewReader = reader.getRoot<Schema::InventoryViewComponent>();
  auto self = component.cast<InventoryViewComponent>();

  self->inventoryEntityId = id.resolve(viewReader.getInventoryEntityId());
  self->name = viewReader.getName();

  auto posReader = viewReader.getPos();
  self->pos = Vec2(posReader.getX(), posReader.getY());

  self->isVisible = viewReader.getIsVisible();
  self->isInteractable = viewReader.getIsInteractable();

  auto paddingReader = viewReader.getPadding();
  self->padding = Vec2(paddingReader.getX(), paddingReader.getY());

  self->slotSize = viewReader.getSlotSize();
  self->rounding = viewReader.getRounding();

  self->windowBackgroundColor = unpackColor(viewReader.getWindowBackgroundColor());
  self->borderColor = unpackColor(viewReader.getBorderColor());
  self->emptySlotColor = unpackColor(viewReader.getEmptySlotColor());
  self->textColor = unpackColor(viewReader.getTextColor());
  self->hoverSlotColor = unpackColor(viewReader.getHoverSlotColor());
  self->dragHoverSlotColor = unpackColor(viewReader.getDragHoverSlotColor());
}

void InventoryViewComponent::fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& jsonValue) {
  auto self = component.cast<InventoryViewComponent>();
  auto compJson = jsonValue.as<JSchema::InventoryViewComponent>();

  if (compJson->hasName())
    self->name = compJson->getName();

  if (compJson->hasPos()) {
    auto posJson = compJson->getPos();
    if (posJson.hasX())
      self->pos.x = posJson.getX();
    if (posJson.hasY())
      self->pos.y = posJson.getY();
  }

  if (compJson->hasIsVisible())
    self->isVisible = compJson->getIsVisible();

  if (compJson->hasIsInteractable())
    self->isInteractable = compJson->getIsInteractable();

  if (compJson->hasPadding()) {
    auto padJson = compJson->getPadding();
    if (padJson.hasX())
      self->padding.x = static_cast<float>(padJson.getX());
    if (padJson.hasY())
      self->padding.y = static_cast<float>(padJson.getY());
  }

  if (compJson->hasSlotSize())
    self->slotSize = static_cast<float>(compJson->getSlotSize());

  if (compJson->hasRounding())
    self->rounding = static_cast<float>(compJson->getRounding());

  // Helper lambda to parse color from JSchema [R,G,B,A] int arrays
  auto parseColor = [](const std::vector<int64_t>& arr) -> glm::vec4 {
    if (arr.size() == 4) {
      return glm::vec4((int)arr[0], (int)arr[1], (int)arr[2], (int)arr[3]);
    }
    return gui::BLACK;
  };

  if (compJson->hasWindowBackgroundColor())
    self->windowBackgroundColor = parseColor(compJson->getWindowBackgroundColor());

  if (compJson->hasBorderColor())
    self->borderColor = parseColor(compJson->getBorderColor());

  if (compJson->hasEmptySlotColor())
    self->emptySlotColor = parseColor(compJson->getEmptySlotColor());

  if (compJson->hasTextColor())
    self->textColor = parseColor(compJson->getTextColor());

  if (compJson->hasHoverSlotColor())
    self->hoverSlotColor = parseColor(compJson->getHoverSlotColor());

  if (compJson->hasDragHoverSlotColor())
    self->dragHoverSlotColor = parseColor(compJson->getDragHoverSlotColor());
}

InventoryViewComponent::~InventoryViewComponent() {
  // Ensure we clean up the TGUI window if this component is destroyed
  if (window) {
    window->removeAll();
    if (window->getParent())
      window->getParent()->remove(window);
    window = nullptr;
  }
  if (tooltipPanel && tooltipPanel->getParent())
    tooltipPanel->getParent()->remove(tooltipPanel);
}

void InventoryViewComponent::update(EntityPtr inventoryEntity, const gui::Gui::Ptr& gui) {
  auto invViewComp = inventoryEntity.get<InventoryViewComponent>();

  if (!inventoryEntity || !inventoryEntity.has<InventoryComponent>()) {
    return; // Inventory doesn't exist or doesn't have component
  }
  
  IWorldPtr world = inventoryEntity.world();
  auto invComp = inventoryEntity.get<InventoryComponent>();
  if(invComp->items.size() != invComp->cols * invComp->rows) {
    // Inventory is invalid, access would cause a crash.
    // This is usually right after deserialization and should
    // fix itself next game tick.
    return;
  }

  invViewComp->inventoryEntityId = inventoryEntity;

  // Check if inventory checksum changed (rebuild UI if it did)
  if (InventoryViewComponent::hasVisualConfigChanged(invViewComp, invComp->cols, invComp->rows, invViewComp->prevVisualChecksum)) {
    invViewComp->prevVisualChecksum = InventoryViewComponent::calculateVisualChecksum(invViewComp, invComp->cols, invComp->rows);
    
    // Destroy existing window and UI elements if grid changed
    if (invViewComp->window) {
      gui->remove(invViewComp->window);
      invViewComp->window = nullptr;
      invViewComp->slotBackgrounds.clear();
      invViewComp->slotPictures.clear();
      invViewComp->slotLabels.clear();
    }
  }

  // Create window if it doesn't exist
  if (!invViewComp->window) {
    invViewComp->window = gui::ChildWindow::create();
    invViewComp->window->title = invViewComp->name;
    invViewComp->window->pos = glm::vec2(invViewComp->pos.x, invViewComp->pos.y);
    invViewComp->window->closeBehaviour = gui::CloseBehaviour::Hide;
    invViewComp->window->titleBarHeight = 15;
    invViewComp->window->titleBarColor = invViewComp->windowBackgroundColor; 
    invViewComp->window->backgroundColor = invViewComp->windowBackgroundColor;  
    invViewComp->window->borderColor = getContrastingColor(invViewComp->windowBackgroundColor);   
    invViewComp->window->border = glm::vec2(1);

    gui->add(invViewComp->window);

    // Create slot UI elements using inventory's actual rows/cols
    invViewComp->slotBackgrounds.resize(invComp->cols * invComp->rows);
    invViewComp->slotPictures.resize(invComp->cols * invComp->rows);
    invViewComp-> slotLabels.resize(invComp->cols * invComp->rows);

    static constexpr float slotIconPadding = 4.0f; // Padding inside slot for icon

    for (u16 y = 0; y < invComp->rows; ++y) {
      for (u16 x = 0; x < invComp->cols; ++x) {
        size_t index = y * invComp->cols + x;
        float posX = x * invViewComp->slotSize + invViewComp->padding.x;
        float posY = y * invViewComp->slotSize + invViewComp->padding.y;
        glm::u16vec2 slotPos(x, y);

        // Create background panel
        auto bg = gui::Panel::create();
        bg->size = glm::vec2(invViewComp->slotSize, invViewComp->slotSize);
        bg->pos = glm::vec2(posX, posY);
        bg->border = glm::vec2(2);
        bg->borderColor = invViewComp->borderColor;
        bg->backgroundColor = invViewComp->emptySlotColor;
        // bg->rounding = invViewComp->rounding; TODO

        invViewComp->window->add(bg);
        invViewComp->slotBackgrounds[index] = bg;

        // Create picture for item icon
        auto picture = gui::Picture::create();
        picture->size = glm::vec2(invViewComp->slotSize - slotIconPadding * 2);
        picture->pos = glm::vec2(posX + slotIconPadding, posY + slotIconPadding);
        invViewComp->window->add(picture);
        invViewComp->slotPictures[index] = picture;

        // Create label for count/text
        auto label = gui::Label::create();
        label->size = glm::vec2(invViewComp->slotSize);
        label->pos = glm::vec2(posX, posY);
        label->textColor = invViewComp->textColor;
        invViewComp->window->add(label);
        invViewComp->slotLabels[index] = label;

        label->onMouseEnter = 
          [inv = inventoryEntity, index]() {
          auto invViewComp = inv.get<InventoryViewComponent>();
          if (invViewComp->isInteractable) {
            invViewComp->slotBackgrounds[index]->backgroundColor = isDragging ? invViewComp->dragHoverSlotColor : invViewComp->hoverSlotColor;
            InventoryViewComponent::showTooltip(inv, index);
          }
        };

        label->onMouseLeave =
          [inv = inventoryEntity, index]() {
          auto invViewComp = inv.get<InventoryViewComponent>();
          if (invViewComp->isInteractable) {
            invViewComp->slotBackgrounds[index]->backgroundColor = invViewComp->emptySlotColor;
            InventoryViewComponent::hideTooltip(invViewComp);
          }
        };

        // Add mouse event handlers for drag and drop
        label->onMousePress = 
          [inv = inventoryEntity, slotPos]() {
          auto invViewComp = inv.get<InventoryViewComponent>();
          if (invViewComp->isInteractable) {
            InventoryViewComponent::onSlotMouseDown(inv, slotPos);
          }
        };
        
        label->onMouseRelease =
          [inv = inventoryEntity, slotPos]() {
          auto invViewComp = inv.get<InventoryViewComponent>();
          if (invViewComp->isInteractable) {
            InventoryViewComponent::onSlotMouseUp(inv, slotPos);
          }
        };

        label->onDoubleClick = 
          [inv = inventoryEntity, slotPos]() {
          auto invViewComp = inv.get<InventoryViewComponent>();
          if (invViewComp->isInteractable) {
            InventoryViewComponent::onSlotDoubleClick(inv, slotPos);
          }
        };
      }
    }

    // Set window size
      invViewComp->window->size = glm::vec2(
        invComp->cols * invViewComp->slotSize + invViewComp->padding.x * 2,
        invComp->rows * invViewComp->slotSize + invViewComp->padding.y * 2
      );

      invViewComp->window->isVisible = invViewComp->isVisible;
      invViewComp->window->onPositionChange = [inv = inventoryEntity]() {
        auto invViewComp = inv.get<InventoryViewComponent>();
        invViewComp->pos = Vec2(invViewComp->window->pos->x, invViewComp->window->pos->y);
      };
      invViewComp->window->onClose = [inv = inventoryEntity]() {
        auto invViewComp = inv.get<InventoryViewComponent>();
        invViewComp->isVisible = false;
      };

      // Create tooltip panel (initially hidden)
      if (!invViewComp->tooltipPanel) {
        glm::vec2 windowSize = *invViewComp->window->size;
        float tooltipWidth = windowSize.x;
        float tooltipHeight = windowSize.y * 0.7f; // 70% of window height

        invViewComp->tooltipPanel = gui::Panel::create();
        invViewComp->tooltipPanel->size = glm::vec2(tooltipWidth, tooltipHeight);
        invViewComp->tooltipPanel->backgroundColor = invViewComp->windowBackgroundColor;
        invViewComp->tooltipPanel->borderColor = invViewComp->borderColor;
        invViewComp->tooltipPanel->border = glm::vec2(2);
        gui->add(invViewComp->tooltipPanel);

        // Tooltip icon (scale with window)
        float iconSize = invViewComp->slotSize * 0.8f;
        invViewComp->tooltipIcon = gui::Picture::create();
        invViewComp->tooltipIcon->size = glm::vec2(iconSize, iconSize);
        invViewComp->tooltipIcon->pos = glm::vec2(5, 5);
        invViewComp->tooltipPanel->add(invViewComp->tooltipIcon);

        // Tooltip name label
        invViewComp->tooltipName = gui::Label::create();
        invViewComp->tooltipName->size = glm::vec2(tooltipWidth - iconSize - 15, tooltipHeight * 0.25f);
        invViewComp->tooltipName->pos = glm::vec2(iconSize + 10, 5);
        invViewComp->tooltipName->textColor = invViewComp->textColor;
        invViewComp->tooltipName->textSize = 12;
        invViewComp->tooltipPanel->add(invViewComp->tooltipName);

        invViewComp->tooltipDescription = gui::Label::create();
        invViewComp->tooltipDescription->size = glm::vec2(tooltipWidth - 10, tooltipHeight * 0.5f);
        invViewComp->tooltipDescription->pos = glm::vec2(5, iconSize + 10);
        invViewComp->tooltipDescription->textColor = invViewComp->textColor;
        invViewComp->tooltipDescription->borderColor = invViewComp->borderColor;
        invViewComp->tooltipDescription->border = glm::vec2(1);
        invViewComp->tooltipDescription->textSize = 11;
        // No scrollbar in custom gui
        invViewComp->tooltipPanel->add(invViewComp->tooltipDescription);

        // Tooltip unique status label
        invViewComp->tooltipUnique = gui::Label::create();
        invViewComp->tooltipUnique->size = glm::vec2(tooltipWidth - 10, tooltipHeight * 0.15f);
        invViewComp->tooltipUnique->textColor = invViewComp->textColor;
        invViewComp->tooltipUnique->textSize = 11;
        // No auto layout or scrollbar
        invViewComp->tooltipPanel->add(invViewComp->tooltipUnique);

        invViewComp->tooltipPanel->isVisible = false;
      }
    }

    if(invViewComp->window->isVisible != invViewComp->isVisible)
      invViewComp->window->isVisible = invViewComp->isVisible;
    if(invViewComp->window->title != invViewComp->name)
      invViewComp->window->title = invViewComp->name;
    if(*invViewComp->window->pos != glm::vec2(invViewComp->pos.x, invViewComp->pos.y))
      *invViewComp->window->pos = glm::vec2(invViewComp->pos.x, invViewComp->pos.y);

    // Update slot contents
    for (u16 y = 0; y < invComp->rows; ++y) {
      for (u16 x = 0; x < invComp->cols; ++x) {
        size_t index = y * invComp->cols + x;
        const ItemStackComponent& slot = invComp->getSlot(x, y);

        auto bg = invViewComp->slotBackgrounds[index];
        auto picture = invViewComp->slotPictures[index];
        auto label = invViewComp->slotLabels[index];

        if (slot.itemId == 0) {
          // Empty slot
          picture->texture = TextureId::null();
          label->text = "";

          glm::vec4 curColor = bg->backgroundColor;
          if(curColor != invViewComp->emptySlotColor && curColor != invViewComp->hoverSlotColor && curColor != invViewComp->dragHoverSlotColor) {
            bg->backgroundColor = invViewComp->emptySlotColor;
          }
        } else {
          // Filled slot
          auto itemEntity = world->getEntity(slot.itemId);
          if (itemEntity && itemEntity.has<ItemComponent>()) {
            auto itemComp = itemEntity.get<ItemComponent>();

            // Set icon
            picture->texture = itemComp->icon;

            // Set count label (only for stackables with count > 1)
            if (!itemComp->isUnique && slot.count > 1) {
              label->text = std::to_string(slot.count);
            } else {
              label->text = "";
            }

            // Set background color (slightly darker to indicate filled)
            bg->backgroundColor = glm::vec4(60, 60, 60, 255);
          }
        }

        // Active placement slot handling
        if (isPlacementMode && activePlacementEntity == invViewComp->inventoryEntityId &&
            activePlacementSlot.x == x && activePlacementSlot.y == y) {
          if (slot.itemId == 0) {
            isPlacementMode = false;
            activePlacementEntity = 0;
          } else {
            bg->backgroundColor = glm::vec4(0, 180, 80, 255);
          }
        }
      }
    }

  // Handle mouse release outside inventory bounds
  auto& eventModule = world->getContext<EventModule>();
  Vec2 mousePos = eventModule.getMouseCoords();
  glm::vec2 mousePosVec(mousePos.x, mousePos.y);
  
  // Check if left mouse button is currently pressed
  Uint32 mouseState = SDL_GetMouseState(nullptr, nullptr);
  bool isMouseButtonPressed = (mouseState & SDL_BUTTON_MASK(SDL_BUTTON_LEFT)) != 0;
  
  if (isDragging && wasMouseButtonPressedLastFrame && !isMouseButtonPressed) {
    // Mouse was released - check if it's outside the inventory window
    if (!InventoryViewComponent::isPointInWindowBounds(invViewComp, mousePosVec)) {
      // Mouse released outside inventory - drop the item to the world
      if(eventModule.isKeyHeld(Key::LeftShift)) {
        // Place the item
        InventoryViewComponent::placeItemToWorld(inventoryEntity);
      } else {
        InventoryViewComponent::dropItemToWorld(inventoryEntity);
      }
      isDragging = false;
      dragSourceEntity = 0;
    }
  }

  // Handle active placement mode (click/drag to place tiles in world)
  if (isPlacementMode && activePlacementEntity == invViewComp->inventoryEntityId && isMouseButtonPressed) {
    if (!InventoryViewComponent::isPointInWindowBounds(invViewComp, mousePosVec)) {
      Vec2 placePos = world->getContext<Render2D>().getWorldCoords(mousePos);

      InventoryManager invMgr;
      invMgr.placeItem(world, activePlacementEntity,
                       activePlacementSlot.x, activePlacementSlot.y, placePos);
    }
  }
  
  // Update mouse button state for next frame
  wasMouseButtonPressedLastFrame = isMouseButtonPressed;
}

void InventoryViewComponent::onSlotMouseDown(EntityPtr inventoryEntity, glm::u16vec2 slotPos) {
  // Check if slot is empty - only start drag if there's an item
  if (!inventoryEntity || !inventoryEntity.has<InventoryComponent>()) {
    return;
  }

  auto invComp = inventoryEntity.get<InventoryComponent>();
  const ItemStackComponent& slot = invComp->getSlot(slotPos.x, slotPos.y);

  if (slot.itemId != 0) {
    isDragging = true;
    dragStartSlot = slotPos;
    dragSourceEntity = inventoryEntity;
  }
}

void InventoryViewComponent::onSlotMouseUp(EntityPtr inventoryEntity, glm::u16vec2 dropSlot) {
  if (!isDragging) {
    return;
  }

  isDragging = false;

  // Only perform drop if we have a valid source
  if (dragSourceEntity != 0) {
    performItemDrop(inventoryEntity, dragSourceEntity, dragStartSlot, dropSlot);
  }

  dragSourceEntity = 0;
}

void InventoryViewComponent::onSlotDoubleClick(EntityPtr inventoryEntity, glm::u16vec2 slotPos) {
  if (!inventoryEntity || !inventoryEntity.has<InventoryComponent>()) {
    return;
  }

  auto invComp = inventoryEntity.get<InventoryComponent>();
  const ItemStackComponent& slot = invComp->getSlot(slotPos.x, slotPos.y);

  if (slot.itemId == 0) {
    return;
  }

  // Check if item is placeable
  IWorldPtr world = inventoryEntity.world();
  auto itemEntity = world->getEntity(slot.itemId);
  if (!itemEntity || !itemEntity.has<ItemPlaceableComponent>()) {
    return;
  }

  auto invViewComp = inventoryEntity.get<InventoryViewComponent>();

  // Toggle: if already active on this slot, deactivate
  if (isPlacementMode && activePlacementEntity == invViewComp->inventoryEntityId && activePlacementSlot == slotPos) {
    isPlacementMode = false;
    activePlacementEntity = 0;
    return;
  }

  // Activate placement mode
  isPlacementMode = true;
  activePlacementSlot = slotPos;
  activePlacementEntity = invViewComp->inventoryEntityId;

  // Cancel any ongoing drag
  isDragging = false;
  dragSourceEntity = 0;
}

void InventoryViewComponent::performItemDrop(EntityPtr inventoryEntity, EntityId sourceEntity, 
                                             glm::u16vec2 sourceSlot, glm::u16vec2 targetSlot) {
  // Use InventoryManager to perform the move
  InventoryManager invMgr;

  // Source and target inventory entity IDs
  EntityId srcInvId = sourceEntity;
  EntityId dstInvId = inventoryEntity;

  // Perform the move operation
  // moveItems intelligently merges stacks or swaps based on item types
  invMgr.moveItems(inventoryEntity.world(), srcInvId, sourceSlot.x, sourceSlot.y,
                   dstInvId, targetSlot.x, targetSlot.y, 0); // 0 = move all
}

u32 InventoryViewComponent::calculateVisualChecksum(RefT<InventoryViewComponent> self, u32 sizeX, u32 sizeY) {
  u32 hash = 0;
  
  // Hash numeric properties
  hash = ((hash << 5) + hash) ^ std::hash<float>()(self->padding.x);
  hash = ((hash << 5) + hash) ^ std::hash<float>()(self->padding.y);
  hash = ((hash << 5) + hash) ^ std::hash<float>()(self->slotSize);
  hash = ((hash << 5) + hash) ^ std::hash<float>()(self->rounding);
  
  // Hash color values (RGBA)
  auto hashColor = [](u32& h, const glm::vec4& color) {
    h = ((h << 5) + h) ^ (static_cast<u32>(color.r) << 24);
    h = ((h << 5) + h) ^ (static_cast<u32>(color.g) << 16);
    h = ((h << 5) + h) ^ (static_cast<u32>(color.b) << 8);
    h = ((h << 5) + h) ^ static_cast<u32>(color.a);
  };
  
  hashColor(hash, self->windowBackgroundColor);
  hashColor(hash, self->borderColor);
  hashColor(hash, self->emptySlotColor);
  hashColor(hash, self->textColor);
  hashColor(hash, self->hoverSlotColor);
  hashColor(hash, self->dragHoverSlotColor);
  
  // Hash cached grid dimensions (indicates inventory size)
  hash = ((hash << 5) + hash) ^ sizeX;
  hash = ((hash << 5) + hash) ^ sizeY;
  
  return hash;
}

bool InventoryViewComponent::hasVisualConfigChanged(RefT<InventoryViewComponent> self, u32 sizeX, u32 sizeY, u32 previousChecksum) {
  return calculateVisualChecksum(self, sizeX, sizeY) != previousChecksum;
}


void InventoryViewComponent::showTooltip(EntityPtr inventoryEntity, size_t slotIndex) {
  auto invViewComp = inventoryEntity.get<InventoryViewComponent>();
  if (!inventoryEntity || !inventoryEntity.has<InventoryComponent>() || !invViewComp->tooltipPanel) {
    return;
  }

  auto invComp = inventoryEntity.get<InventoryComponent>();
  if (slotIndex >= invComp->items.size()) {
    return;
  }

  const ItemStackComponent& slot = invComp->items[slotIndex];
  if (slot.itemId == 0) {
    invViewComp->tooltipPanel->isVisible = false;
    return;
  }

  IWorldPtr world = inventoryEntity.world();
  auto itemEntity = world->getEntity(slot.itemId);
  if (!itemEntity || !itemEntity.has<ItemComponent>()) {
    invViewComp->tooltipPanel->isVisible = false;
    return;
  }

  auto itemComp = itemEntity.get<ItemComponent>();

  // Update tooltip content
  invViewComp->tooltipIcon->texture = itemComp->icon;
  invViewComp->tooltipName->text = itemComp->name + " (" + std::to_string(slot.itemId) + ")";
  invViewComp->tooltipDescription->text = itemComp->description.empty() ? "No description" : itemComp->description + " has ItemPlaceable: " + (itemEntity.has<ItemPlaceableComponent>() ? "Yes" : "No");
  invViewComp->tooltipUnique->text = itemComp->isUnique ? "Unique Item" : "Stackable";

  // Position tooltip below the inventory window
  if (invViewComp->window) {
    glm::vec2 windowPos = *invViewComp->window->pos;
    glm::vec2 windowSize = *invViewComp->window->size;
    invViewComp->tooltipPanel->pos = glm::vec2(windowPos.x, windowPos.y + windowSize.y + 10);
  }

  invViewComp->tooltipPanel->isVisible = true;
}

void InventoryViewComponent::hideTooltip(RefT<InventoryViewComponent> self) {
  if (self->tooltipPanel) {
    self->tooltipPanel->isVisible = false;
  }
}

bool InventoryViewComponent::isPointInWindowBounds(RefT<InventoryViewComponent> self, const glm::vec2& worldPos) {
  if (!self->window) {
    return false;
  }

  return self->window->contains(worldPos);
}

void InventoryViewComponent::placeItemToWorld(EntityPtr inventoryEntity) {
  if (!isDragging || dragSourceEntity == 0 || !inventoryEntity) {
    return;
  }

  IWorldPtr world = inventoryEntity.world();
  // Get the source inventory and item information
  auto srcEntity = world->getEntity(dragSourceEntity);
  if (!srcEntity || !srcEntity.has<InventoryComponent>()) {
    return;
  }

  auto srcInvComp = srcEntity.get<InventoryComponent>();
  const ItemStackComponent& sourceSlot = srcInvComp->getSlot(dragStartSlot.x, dragStartSlot.y);

  if (sourceSlot.itemId == 0) {
    return; // Empty slot, nothing to place
  }

  // Get the item entity and check if it's placeable
  auto itemEntity = world->getEntity(sourceSlot.itemId);
  if (!itemEntity || !itemEntity.has<ItemPlaceableComponent>()) {
    return; // Item is not placeable
  }

  // Get mouse position to place at
  Vec2 placePos = world->getContext<EventModule>().getMouseWorldPos();

  // Remove the item from inventory after placing
  InventoryManager invMgr;
  invMgr.placeItem(world, dragSourceEntity, 
                  dragStartSlot.x, dragStartSlot.y, placePos);
}

void InventoryViewComponent::dropItemToWorld(EntityPtr inventoryEntity) {
  if (!isDragging || dragSourceEntity == 0 || !inventoryEntity) {
    return;
  }

  IWorldPtr world = inventoryEntity.world();
  // Get the source inventory and item information
  auto srcEntity = world->getEntity(dragSourceEntity);
  if (!srcEntity || !srcEntity.has<InventoryComponent>()) {
    return;
  }

  auto srcInvComp = srcEntity.get<InventoryComponent>();
  const ItemStackComponent& sourceSlot = srcInvComp->getSlot(dragStartSlot.x, dragStartSlot.y);

  if (sourceSlot.itemId == 0) {
    return; // Empty slot, nothing to drop
  }

  // Get mouse position from EventModule
  Vec2 dropPos = world->getContext<EventModule>().getMouseWorldPos();

  // Use InventoryManager to drop the item
  InventoryManager invMgr;
  invMgr.dropItem(world, dragSourceEntity, 
                  dragStartSlot.x, dragStartSlot.y, dropPos, sourceSlot.count);
}

RAMPAGE_END