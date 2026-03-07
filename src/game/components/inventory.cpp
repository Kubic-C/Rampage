#include "inventory.hpp"
#include "../systems/inventory.hpp"
#include "../../event/module.hpp"
#include "../../render/module.hpp"
#include <SDL3/SDL.h>

RAMPAGE_START

// Static member definitions for global drag state
bool InventoryViewComponent::isDragging = false;
glm::u16vec2 InventoryViewComponent::dragStartSlot = {0, 0};
InventoryViewComponent* InventoryViewComponent::dragSourceView = nullptr;
bool InventoryViewComponent::wasMouseButtonPressedLastFrame = false;

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

tgui::Color getContrastingColor(const tgui::Color& color) {
    float r = color.getRed();
    float g = color.getGreen();
    float b = color.getBlue();
    
    // Check if color is grey (all RGB values are similar)
    float maxDiff = std::max({r, g, b}) - std::min({r, g, b});
    bool isGrey = maxDiff < 30;  // Threshold for "grey-like"
    
    if (isGrey) {
        // For grey, use brightness to pick white or black
        float brightness = (0.299f * r + 0.587f * g + 0.114f * b) / 255.0f;
        return brightness < 0.5f ? tgui::Color::White : tgui::Color::Black;
    } else {
        // For colored backgrounds, invert
        return tgui::Color(255 - (unsigned int)r, 
                          255 - (unsigned int)g, 
                          255 - (unsigned int)b, 
                          color.getAlpha());
    }
}

// Helper functions to convert color to/from packed UInt32
inline u32 packColor(const tgui::Color& color) {
  return (static_cast<u32>(color.getRed()) << 24) |
         (static_cast<u32>(color.getGreen()) << 16) |
         (static_cast<u32>(color.getBlue()) << 8) |
         static_cast<u32>(color.getAlpha());
}

inline tgui::Color unpackColor(u32 packed) {
  return tgui::Color(
    (packed >> 24) & 0xFF,
    (packed >> 16) & 0xFF,
    (packed >> 8) & 0xFF,
    packed & 0xFF
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
  auto parseColor = [](const std::vector<int64_t>& arr) -> tgui::Color {
    if (arr.size() == 4) {
      return tgui::Color((int)arr[0], (int)arr[1], (int)arr[2], (int)arr[3]);
    }
    return tgui::Color::Black;
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
    window->removeAllWidgets();
    if (window->getParent())
      window->getParent()->remove(window);
    window = nullptr;
  }
  if (tooltipPanel && tooltipPanel->getParent())
    tooltipPanel->getParent()->remove(tooltipPanel);
}

void InventoryViewComponent::update(EntityPtr inventoryEntity, tgui::Gui& gui) {
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
  if (InventoryViewComponent::hasVisualConfigChanged(*invViewComp, invComp->cols, invComp->rows, invViewComp->prevVisualChecksum)) {
    invViewComp->prevVisualChecksum = InventoryViewComponent::calculateVisualChecksum(*invViewComp, invComp->cols, invComp->rows);
    
    // Destroy existing window and UI elements if grid changed
    if (invViewComp->window) {
      gui.remove(invViewComp->window);
      invViewComp->window = nullptr;
      invViewComp->slotBackgrounds.clear();
      invViewComp->slotPictures.clear();
      invViewComp->slotLabels.clear();
    }
  }

  // Create window if it doesn't exist
  if (!invViewComp->window) {
    invViewComp->window = tgui::ChildWindow::create();
    invViewComp->window->setTitle(invViewComp->name);
    invViewComp->window->setPosition(invViewComp->pos.x, invViewComp->pos.y);
    invViewComp->window->setCloseBehavior(tgui::ChildWindow::CloseBehavior::Hide);

    invViewComp->window->getRenderer()->setTitleBarHeight(15);
    invViewComp->window->getRenderer()->setTitleBarColor(invViewComp->windowBackgroundColor); 
    invViewComp->window->getRenderer()->setTitleColor(getContrastingColor(invViewComp->windowBackgroundColor));
    invViewComp->window->getRenderer()->setBackgroundColor(invViewComp->windowBackgroundColor);  
    invViewComp->window->getRenderer()->setBorderColor(getContrastingColor(invViewComp->windowBackgroundColor));   
    invViewComp->window->getRenderer()->setBorders({1, 1, 1, 1});
    invViewComp->window->getRenderer()->setBorderBelowTitleBar(0); 

    gui.add(invViewComp->window);

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
        auto bg = tgui::Panel::create();
        bg->setSize(invViewComp->slotSize, invViewComp->slotSize);
        bg->setPosition(posX, posY);
        bg->getRenderer()->setBorders(2);
        bg->getRenderer()->setBorderColor(invViewComp->borderColor);
        bg->getRenderer()->setBackgroundColor(invViewComp->emptySlotColor);
        bg->getRenderer()->setRoundedBorderRadius(invViewComp->rounding);

        invViewComp->window->add(bg);
        invViewComp->slotBackgrounds[index] = bg;

        // Create picture for item icon
        auto picture = tgui::Picture::create();
        picture->setSize(invViewComp->slotSize - slotIconPadding * 2, invViewComp->slotSize - slotIconPadding * 2);
        picture->setPosition(posX + slotIconPadding, posY + slotIconPadding);
        invViewComp->window->add(picture);
        invViewComp->slotPictures[index] = picture;

        // Create label for count/text
        auto label = tgui::Label::create();
        label->setSize(invViewComp->slotSize, invViewComp->slotSize);
        label->setPosition(posX, posY);
        label->getRenderer()->setTextColor(invViewComp->textColor);
        invViewComp->window->add(label);
        invViewComp->slotLabels[index] = label;

        label->onMouseEnter([inv = inventoryEntity, index]() {
          auto invViewComp = inv.get<InventoryViewComponent>();
          if (invViewComp->isInteractable) {
            invViewComp->slotBackgrounds[index]->getRenderer()->setBackgroundColor(isDragging ? invViewComp->dragHoverSlotColor : invViewComp->hoverSlotColor);
            InventoryViewComponent::showTooltip(inv, index);
          }
        });

        label->onMouseLeave([inv = inventoryEntity, index]() {
          auto invViewComp = inv.get<InventoryViewComponent>();
          if (invViewComp->isInteractable) {
            invViewComp->slotBackgrounds[index]->getRenderer()->setBackgroundColor(invViewComp->emptySlotColor);
            InventoryViewComponent::hideTooltip(*invViewComp);
          }
        });

        // Add mouse event handlers for drag and drop
        label->onMousePress([inv = inventoryEntity, slotPos]() {
          auto invViewComp = inv.get<InventoryViewComponent>();
          if (invViewComp->isInteractable) {
            InventoryViewComponent::onSlotMouseDown(inv, slotPos);
          }
        });
        
        label->onMouseRelease([inv = inventoryEntity, slotPos]() {
          auto invViewComp = inv.get<InventoryViewComponent>();
          if (invViewComp->isInteractable) {
            InventoryViewComponent::onSlotMouseUp(inv, slotPos);
          }
        });
      }
    }

    // Set window size
    tgui::Vector2f windowPadding = invViewComp->window->getSize() - invViewComp->window->getClientSize();
    invViewComp->window->setSize(
      invComp->cols * invViewComp->slotSize + windowPadding.x + invViewComp->padding.x * 2,
      invComp->rows * invViewComp->slotSize + windowPadding.y + invViewComp->padding.y * 2
    );

    invViewComp->window->setVisible(invViewComp->isVisible);
    invViewComp->window->onPositionChange([inv = inventoryEntity]() {
      auto invViewComp = inv.get<InventoryViewComponent>();
      invViewComp->pos = Vec2(invViewComp->window->getPosition().x, invViewComp->window->getPosition().y);
    });
    invViewComp->window->onClose([inv = inventoryEntity]() {
      auto invViewComp = inv.get<InventoryViewComponent>();
      invViewComp->isVisible = false;
    });

    // Create tooltip panel (initially hidden)
    if (!invViewComp->tooltipPanel) {
      tgui::Vector2f windowSize = invViewComp->window->getSize();
      float tooltipWidth = windowSize.x;
      float tooltipHeight = windowSize.y * 0.7f; // 70% of window height

      invViewComp->tooltipPanel = tgui::Panel::create();
      invViewComp->tooltipPanel->setSize(tooltipWidth, tooltipHeight);
      invViewComp->tooltipPanel->getRenderer()->setBackgroundColor(invViewComp->windowBackgroundColor);
      invViewComp->tooltipPanel->getRenderer()->setBorderColor(invViewComp->borderColor);
      invViewComp->tooltipPanel->getRenderer()->setBorders({2, 2, 2, 2});
      gui.add(invViewComp->tooltipPanel);

      // Tooltip icon (scale with window)
      float iconSize = invViewComp->slotSize * 0.8f;
      invViewComp->tooltipIcon = tgui::Picture::create();
      invViewComp->tooltipIcon->setSize(iconSize, iconSize);
      invViewComp->tooltipIcon->setPosition(5, 5);
      invViewComp->tooltipPanel->add(invViewComp->tooltipIcon);

      // Tooltip name label
      invViewComp->tooltipName = tgui::Label::create();
      invViewComp->tooltipName->setSize(tooltipWidth - iconSize - 15, tooltipHeight * 0.25f);
      invViewComp->tooltipName->setPosition(iconSize + 10, 5);
      invViewComp->tooltipName->getRenderer()->setTextColor(invViewComp->textColor);
      invViewComp->tooltipName->setTextSize(12);
      invViewComp->tooltipPanel->add(invViewComp->tooltipName);

      invViewComp->tooltipDescription = tgui::Label::create();
      invViewComp->tooltipDescription->setSize(tooltipWidth - 10, tooltipHeight * 0.5f);
      invViewComp->tooltipDescription->setPosition(5, iconSize + 10);
      invViewComp->tooltipDescription->getRenderer()->setTextColor(invViewComp->textColor);
      invViewComp->tooltipDescription->getRenderer()->setBorderColor(invViewComp->borderColor);
      invViewComp->tooltipDescription->getRenderer()->setBorders({1, 1, 1, 1});
      invViewComp->tooltipDescription->setTextSize(11);
      invViewComp->tooltipDescription->getScrollbar()->setPolicy(tgui::Scrollbar::Policy::Never);
      invViewComp->tooltipPanel->add(invViewComp->tooltipDescription);

      // Tooltip unique status label
      invViewComp->tooltipUnique = tgui::Label::create();
      invViewComp->tooltipUnique->setSize(tooltipWidth - 10, tooltipHeight * 0.15f);
      invViewComp->tooltipUnique->getRenderer()->setTextColor(invViewComp->textColor);
      invViewComp->tooltipUnique->setTextSize(11);
      invViewComp->tooltipUnique->setAutoLayout(tgui::AutoLayout::Bottom);
      invViewComp->tooltipUnique->getScrollbar()->setPolicy(tgui::Scrollbar::Policy::Never);
      invViewComp->tooltipPanel->add(invViewComp->tooltipUnique);

      invViewComp->tooltipPanel->setVisible(false);
    }
  }

  if(invViewComp->window->isVisible() != invViewComp->isVisible)
    invViewComp->window->setVisible(invViewComp->isVisible);
  if(invViewComp->window->getTitle() != invViewComp->name)
    invViewComp->window->setTitle(invViewComp->name); 
  if(invViewComp->window->getPosition() != tgui::Vector2f(invViewComp->pos.x, invViewComp->pos.y))
    invViewComp->window->setPosition(invViewComp->pos.x, invViewComp->pos.y);

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
        picture->getRenderer()->setTexture(tgui::Texture());
        label->setText("");

        tgui::Color curColor = bg->getRenderer()->getBackgroundColor();
        if(curColor != invViewComp->emptySlotColor && curColor != invViewComp->hoverSlotColor && curColor != invViewComp->dragHoverSlotColor) {
          bg->getRenderer()->setBackgroundColor(invViewComp->emptySlotColor);
        }
      } else {
        // Filled slot
        auto itemEntity = world->getEntity(slot.itemId);
        if (itemEntity && itemEntity.has<ItemComponent>()) {
          auto itemComp = itemEntity.get<ItemComponent>();

          // Set icon
          picture->getRenderer()->setTexture(itemComp->icon);

          // Set count label (only for stackables with count > 1)
          if (!itemComp->isUnique && slot.count > 1) {
            label->setText(std::to_string(slot.count));
          } else {
            label->setText("");
          }

          // Set background color (slightly darker to indicate filled)
          bg->getRenderer()->setBackgroundColor(tgui::Color(60, 60, 60));
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
    if (!InventoryViewComponent::isPointInWindowBounds(*invViewComp, mousePosVec)) {
      // Mouse released outside inventory - drop the item to the world
      if(eventModule.isKeyHeld(Key::LeftShift)) {
        // Place the item
        InventoryViewComponent::placeItemToWorld(inventoryEntity);
      } else {
        InventoryViewComponent::dropItemToWorld(inventoryEntity);
      }
      isDragging = false;
      dragSourceView = nullptr;
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
    dragSourceView = &(*inventoryEntity.get<InventoryViewComponent>());
  }
}

void InventoryViewComponent::onSlotMouseUp(EntityPtr inventoryEntity, glm::u16vec2 dropSlot) {
  if (!isDragging) {
    return;
  }

  isDragging = false;

  // Only perform drop if we have a valid source view
  if (dragSourceView != nullptr) {
    performItemDrop(inventoryEntity, dragSourceView, dragStartSlot, dropSlot);
  }

  dragSourceView = nullptr;
}

void InventoryViewComponent::performItemDrop(EntityPtr inventoryEntity, InventoryViewComponent* sourceView, 
                                             glm::u16vec2 sourceSlot, glm::u16vec2 targetSlot) {
  // Use InventoryManager to perform the move
  InventoryManager invMgr;

  // Source and target inventory entity IDs
  EntityId srcInvId = sourceView->inventoryEntityId;
  EntityId dstInvId = inventoryEntity;

  // Perform the move operation
  // moveItems intelligently merges stacks or swaps based on item types
  invMgr.moveItems(inventoryEntity.world(), srcInvId, sourceSlot.x, sourceSlot.y,
                   dstInvId, targetSlot.x, targetSlot.y, 0); // 0 = move all
}

u32 InventoryViewComponent::calculateVisualChecksum(const InventoryViewComponent& self, u32 sizeX, u32 sizeY) {
  u32 hash = 0;
  
  // Hash numeric properties
  hash = ((hash << 5) + hash) ^ std::hash<float>()(self.padding.x);
  hash = ((hash << 5) + hash) ^ std::hash<float>()(self.padding.y);
  hash = ((hash << 5) + hash) ^ std::hash<float>()(self.slotSize);
  hash = ((hash << 5) + hash) ^ std::hash<float>()(self.rounding);
  
  // Hash color values (RGBA)
  auto hashColor = [](u32& h, const tgui::Color& color) {
    h = ((h << 5) + h) ^ (static_cast<u32>(color.getRed()) << 24);
    h = ((h << 5) + h) ^ (static_cast<u32>(color.getGreen()) << 16);
    h = ((h << 5) + h) ^ (static_cast<u32>(color.getBlue()) << 8);
    h = ((h << 5) + h) ^ static_cast<u32>(color.getAlpha());
  };
  
  hashColor(hash, self.windowBackgroundColor);
  hashColor(hash, self.borderColor);
  hashColor(hash, self.emptySlotColor);
  hashColor(hash, self.textColor);
  hashColor(hash, self.hoverSlotColor);
  hashColor(hash, self.dragHoverSlotColor);
  
  // Hash cached grid dimensions (indicates inventory size)
  hash = ((hash << 5) + hash) ^ sizeX;
  hash = ((hash << 5) + hash) ^ sizeY;
  
  return hash;
}

bool InventoryViewComponent::hasVisualConfigChanged(const InventoryViewComponent& self, u32 sizeX, u32 sizeY, u32 previousChecksum) {
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
    invViewComp->tooltipPanel->setVisible(false);
    return;
  }

  IWorldPtr world = inventoryEntity.world();
  auto itemEntity = world->getEntity(slot.itemId);
  if (!itemEntity || !itemEntity.has<ItemComponent>()) {
    invViewComp->tooltipPanel->setVisible(false);
    return;
  }

  auto itemComp = itemEntity.get<ItemComponent>();

  // Update tooltip content
  invViewComp->tooltipIcon->getRenderer()->setTexture(itemComp->icon);
  invViewComp->tooltipName->setText(itemComp->name + " (" + std::to_string(slot.itemId) + ")");
  invViewComp->tooltipDescription->setText(itemComp->description.empty() ? "No description" : itemComp->description + " has ItemPlaceable: " + (itemEntity.has<ItemPlaceableComponent>() ? "Yes" : "No") );
  invViewComp->tooltipUnique->setText(itemComp->isUnique ? "Unique Item" : "Stackable");

  // Position tooltip below the inventory window
  if (invViewComp->window) {
    tgui::Vector2f windowPos = invViewComp->window->getPosition();
    tgui::Vector2f windowSize = invViewComp->window->getSize();
    invViewComp->tooltipPanel->setPosition(windowPos.x, windowPos.y + windowSize.y + 10);
  }

  invViewComp->tooltipPanel->setVisible(true);
}

void InventoryViewComponent::hideTooltip(InventoryViewComponent& self) {
  if (self.tooltipPanel) {
    self.tooltipPanel->setVisible(false);
  }
}

bool InventoryViewComponent::isPointInWindowBounds(const InventoryViewComponent& self, const glm::vec2& worldPos) {
  if (!self.window) {
    return false;
  }

  tgui::Vector2f windowPos = self.window->getPosition();
  tgui::Vector2f windowSize = self.window->getSize();

  return worldPos.x >= windowPos.x && worldPos.x <= windowPos.x + windowSize.x &&
         worldPos.y >= windowPos.y && worldPos.y <= windowPos.y + windowSize.y;
}

void InventoryViewComponent::placeItemToWorld(EntityPtr inventoryEntity) {
  if (!isDragging || !dragSourceView || !inventoryEntity) {
    return;
  }

  IWorldPtr world = inventoryEntity.world();
  // Get the source inventory and item information
  auto srcEntity = world->getEntity(dragSourceView->inventoryEntityId);
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
  Vec2 placePos = world->getContext<RenderModule>().getWorldCoords(world->getContext<EventModule>().getMouseCoords());

  // Remove the item from inventory after placing
  InventoryManager invMgr;
  invMgr.placeItem(world, dragSourceView->inventoryEntityId, 
                  dragStartSlot.x, dragStartSlot.y, placePos);
}

void InventoryViewComponent::dropItemToWorld(EntityPtr inventoryEntity) {
  if (!isDragging || !dragSourceView || !inventoryEntity) {
    return;
  }

  IWorldPtr world = inventoryEntity.world();
  // Get the source inventory and item information
  auto srcEntity = world->getEntity(dragSourceView->inventoryEntityId);
  if (!srcEntity || !srcEntity.has<InventoryComponent>()) {
    return;
  }

  auto srcInvComp = srcEntity.get<InventoryComponent>();
  const ItemStackComponent& sourceSlot = srcInvComp->getSlot(dragStartSlot.x, dragStartSlot.y);

  if (sourceSlot.itemId == 0) {
    return; // Empty slot, nothing to drop
  }

  // Get mouse position from EventModule
  Vec2 dropPos = world->getContext<RenderModule>().getWorldCoords(world->getContext<EventModule>().getMouseCoords());

  // Use InventoryManager to drop the item
  InventoryManager invMgr;
  invMgr.dropItem(world, dragSourceView->inventoryEntityId, 
                  dragStartSlot.x, dragStartSlot.y, dropPos, sourceSlot.count);
}

RAMPAGE_END