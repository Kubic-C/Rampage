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

void ItemUseComponent::serialize(capnp::MessageBuilder& builder, Ref component) {
  auto itemUseBuilder = builder.initRoot<Schema::ItemUseComponent>();
  auto self = component.cast<ItemUseComponent>();

  itemUseBuilder.setEntityId(self->entityId);
  itemUseBuilder.setEffectValue(self->effectValue);
  itemUseBuilder.setEffectRadius(self->effectRadius);
  itemUseBuilder.setCooldown(self->cooldown);
  itemUseBuilder.setRemainingCooldown(self->remainingCooldown);
  itemUseBuilder.setMaxCharges(self->maxCharges);
  itemUseBuilder.setCurrentCharges(self->currentCharges);
  itemUseBuilder.setIsActive(self->isActive);
}

void ItemUseComponent::deserialize(capnp::MessageReader& reader, const IdMapper& idMapper, Ref component) {
  auto itemUseReader = reader.getRoot<Schema::ItemUseComponent>();
  auto self = component.cast<ItemUseComponent>();

  self->entityId = idMapper.resolve(itemUseReader.getEntityId());
  self->effectValue = itemUseReader.getEffectValue();
  self->effectRadius = itemUseReader.getEffectRadius();
  self->cooldown = itemUseReader.getCooldown();
  self->remainingCooldown = itemUseReader.getRemainingCooldown();
  self->maxCharges = itemUseReader.getMaxCharges();
  self->currentCharges = itemUseReader.getCurrentCharges();
  self->isActive = itemUseReader.getIsActive();
}

void ItemUseComponent::fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& jsonValue) {
  auto self = component.cast<ItemUseComponent>();
  auto compJson = jsonValue.as<JSchema::ItemUseComponent>();

  // Note: entityId reference will be resolved by the entity loader
  if (compJson->hasEntityId())
    self->entityId = compJson->getEntityId();

  if (compJson->hasEffectValue())
    self->effectValue = compJson->getEffectValue();

  if (compJson->hasEffectRadius())
    self->effectRadius = compJson->getEffectRadius();

  if (compJson->hasCooldown())
    self->cooldown = compJson->getCooldown();

  if (compJson->hasRemainingCooldown())
    self->remainingCooldown = compJson->getRemainingCooldown();

  if (compJson->hasMaxCharges())
    self->maxCharges = compJson->getMaxCharges();

  if (compJson->hasCurrentCharges())
    self->currentCharges = compJson->getCurrentCharges();

  if (compJson->hasIsActive())
    self->isActive = compJson->getIsActive();
}

void ItemComponent::serialize(capnp::MessageBuilder& builder, Ref component) {
  auto itemBuilder = builder.initRoot<Schema::ItemComponent>();
  auto self = component.cast<ItemComponent>();

  itemBuilder.setName(self->name);
  itemBuilder.setDescription(self->description);
  itemBuilder.setMaxStackSize(self->maxStackSize);
  itemBuilder.setStackCost(self->stackCost);
  itemBuilder.setIsUnique(self->isUnique);
  itemBuilder.setIconPath((std::string)self->icon.getId()); // Assuming TGUI's Texture has a method to get the original file path
}

void ItemComponent::deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component) {
  auto itemReader = reader.getRoot<Schema::ItemComponent>();
  auto self = component.cast<ItemComponent>();

  self->name = itemReader.getName();
  self->description = itemReader.getDescription();
  self->maxStackSize = itemReader.getMaxStackSize();
  self->stackCost = itemReader.getStackCost();
  self->isUnique = itemReader.getIsUnique();
  
  std::string pathStr = itemReader.getIconPath().cStr();
  if (std::filesystem::exists(pathStr)) {
    self->icon = tgui::Texture(pathStr); // Load texture from path
  }
}

void ItemComponent::fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& jsonValue) {
  auto self = component.cast<ItemComponent>();
  auto compJson = jsonValue.as<JSchema::ItemComponent>();

  if (compJson->hasName())
    self->name = compJson->getName();

  if (compJson->hasDescription())
    self->description = compJson->getDescription();

  if (compJson->hasMaxStackSize())
    self->maxStackSize = compJson->getMaxStackSize();

  if (compJson->hasStackCost())
    self->stackCost = compJson->getStackCost();

  if (compJson->hasIsUnique())
    self->isUnique = compJson->getIsUnique();

  // Load icon from asset loader if iconAsset is specified
  if (compJson->hasIconAsset()) {
    std::string pathStr = compJson->getIconAsset();
    if (std::filesystem::exists(pathStr)) {
      self->icon = tgui::Texture(pathStr); // Load texture from path
    } else {
      component.getWorld()->getHost().log("Could not load icon asset for ItemComponent: %s for %s", pathStr.c_str(), self->name.c_str());
    }
  }
}

void ItemStackComponent::serialize(capnp::MessageBuilder& builder, Ref component) {
  auto stackBuilder = builder.initRoot<Schema::ItemStackComponent>();
  auto self = component.cast<ItemStackComponent>();

  stackBuilder.setCount(self->count);
  stackBuilder.setItemId(self->itemId);
}

void ItemStackComponent::deserialize(capnp::MessageReader& reader, const IdMapper& idMapper, Ref component) {
  auto stackReader = reader.getRoot<Schema::ItemStackComponent>();
  auto self = component.cast<ItemStackComponent>();

  self->count = stackReader.getCount();
  self->itemId = idMapper.resolve(stackReader.getItemId());
}

void ItemStackComponent::fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& jsonValue) {
  auto self = component.cast<ItemStackComponent>();
  auto compJson = jsonValue.as<JSchema::ItemStackComponent>();

  if (compJson->hasCount())
    self->count = compJson->getCount();

  if (compJson->hasItemId())
    self->itemId = compJson->getItemId();
}

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
      self->padding.x = padJson.getX();
    if (padJson.hasY())
      self->padding.y = padJson.getY();
  }

  if (compJson->hasSlotSize())
    self->slotSize = compJson->getSlotSize();

  if (compJson->hasRounding())
    self->rounding = compJson->getRounding();

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
    window->getParent()->remove(window);
    window = nullptr;
  }
}

void InventoryViewComponent::update(EntityPtr inventoryEntity, tgui::Gui& gui) {
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

  inventoryEntityId = inventoryEntity;

  // Check if inventory checksum changed (rebuild UI if it did)
  if (hasVisualConfigChanged(invComp->cols, invComp->rows, prevVisualChecksum)) {
    prevVisualChecksum = calculateVisualChecksum(invComp->cols, invComp->rows);
    
    // Destroy existing window and UI elements if grid changed
    if (window) {
      gui.remove(window);
      window = nullptr;
      slotBackgrounds.clear();
      slotPictures.clear();
      slotLabels.clear();
    }
  }

  // Create window if it doesn't exist
  if (!window) {
    window = tgui::ChildWindow::create();
    window->setTitle(name);
    window->setPosition(pos.x, pos.y);

    window->getRenderer()->setTitleBarHeight(15);
    window->getRenderer()->setTitleBarColor(windowBackgroundColor); 
    window->getRenderer()->setTitleColor(getContrastingColor(windowBackgroundColor));
    window->getRenderer()->setBackgroundColor(windowBackgroundColor);  
    window->getRenderer()->setBorderColor(getContrastingColor(windowBackgroundColor));   
    window->getRenderer()->setBorders({1, 1, 1, 1});
    window->getRenderer()->setBorderBelowTitleBar(0); 

    gui.add(window);

    // Create slot UI elements using inventory's actual rows/cols
    slotBackgrounds.resize(invComp->cols * invComp->rows);
    slotPictures.resize(invComp->cols * invComp->rows);
    slotLabels.resize(invComp->cols * invComp->rows);

    static constexpr float slotIconPadding = 4.0f; // Padding inside slot for icon

    for (u16 y = 0; y < invComp->rows; ++y) {
      for (u16 x = 0; x < invComp->cols; ++x) {
        size_t index = y * invComp->cols + x;
        float posX = x * slotSize + padding.x;
        float posY = y * slotSize + padding.y;
        glm::u16vec2 slotPos(x, y);

        // Create background panel
        auto bg = tgui::Panel::create();
        bg->setSize(slotSize, slotSize);
        bg->setPosition(posX, posY);
        bg->getRenderer()->setBorders(2);
        bg->getRenderer()->setBorderColor(borderColor);
        bg->getRenderer()->setBackgroundColor(emptySlotColor);
        bg->getRenderer()->setRoundedBorderRadius(rounding);

        window->add(bg);
        slotBackgrounds[index] = bg;

        // Create picture for item icon
        auto picture = tgui::Picture::create();
        picture->setSize(slotSize - slotIconPadding * 2, slotSize - slotIconPadding * 2);
        picture->setPosition(posX + slotIconPadding, posY + slotIconPadding);
        window->add(picture);
        slotPictures[index] = picture;

        // Create label for count/text
        auto label = tgui::Label::create();
        label->setSize(slotSize, slotSize);
        label->setPosition(posX, posY);
        label->getRenderer()->setTextColor(textColor);
        window->add(label);
        slotLabels[index] = label;

        label->onMouseEnter([this, inventoryEntity, index]() {
          if (isInteractable) {
            slotBackgrounds[index]->getRenderer()->setBackgroundColor(isDragging ? dragHoverSlotColor : hoverSlotColor);
            showTooltip(inventoryEntity, index);
          }
        });

        label->onMouseLeave([this, index]() {
          if (isInteractable) {
            slotBackgrounds[index]->getRenderer()->setBackgroundColor(emptySlotColor);
            hideTooltip();
          }
        });

        // Add mouse event handlers for drag and drop
        label->onMousePress([this, slotPos, inventoryEntity]() {
          if (isInteractable) {
            onSlotMouseDown(inventoryEntity, slotPos);
          }
        });
        
        label->onMouseRelease([this, slotPos, inventoryEntity]() {
          if (isInteractable) {
            onSlotMouseUp(inventoryEntity, slotPos);
          }
        });
      }
    }

    // Set window size
    tgui::Vector2f windowPadding = window->getSize() - window->getClientSize();
    window->setSize(
      invComp->cols * slotSize + windowPadding.x + padding.x * 2,
      invComp->rows * slotSize + windowPadding.y + padding.y * 2
    );

    window->setVisible(isVisible);
    window->onPositionChange([this]() {
      pos = Vec2(window->getPosition().x, window->getPosition().y);
    });

    // Create tooltip panel (initially hidden)
    if (!tooltipPanel) {
      tgui::Vector2f windowSize = window->getSize();
      float tooltipWidth = windowSize.x;
      float tooltipHeight = windowSize.y * 0.7f; // 70% of window height

      tooltipPanel = tgui::Panel::create();
      tooltipPanel->setSize(tooltipWidth, tooltipHeight);
      tooltipPanel->getRenderer()->setBackgroundColor(windowBackgroundColor);
      tooltipPanel->getRenderer()->setBorderColor(borderColor);
      tooltipPanel->getRenderer()->setBorders({2, 2, 2, 2});
      gui.add(tooltipPanel);

      // Tooltip icon (scale with window)
      float iconSize = slotSize * 0.8f;
      tooltipIcon = tgui::Picture::create();
      tooltipIcon->setSize(iconSize, iconSize);
      tooltipIcon->setPosition(5, 5);
      tooltipPanel->add(tooltipIcon);

      // Tooltip name label
      tooltipName = tgui::Label::create();
      tooltipName->setSize(tooltipWidth - iconSize - 15, tooltipHeight * 0.25f);
      tooltipName->setPosition(iconSize + 10, 5);
      tooltipName->getRenderer()->setTextColor(textColor);
      tooltipName->setTextSize(12);
      tooltipPanel->add(tooltipName);

      tooltipDescription = tgui::Label::create();
      tooltipDescription->setSize(tooltipWidth - 10, tooltipHeight * 0.5f);
      tooltipDescription->setPosition(5, iconSize + 10);
      tooltipDescription->getRenderer()->setTextColor(textColor);
      tooltipDescription->getRenderer()->setBorderColor(borderColor);
      tooltipDescription->getRenderer()->setBorders({1, 1, 1, 1});
      tooltipDescription->setTextSize(11);
      tooltipDescription->setScrollbarPolicy(tgui::Scrollbar::Policy::Never);
      tooltipPanel->add(tooltipDescription);

      // Tooltip unique status label
      tooltipUnique = tgui::Label::create();
      tooltipUnique->setSize(tooltipWidth - 10, tooltipHeight * 0.15f);
      tooltipUnique->getRenderer()->setTextColor(textColor);
      tooltipUnique->setTextSize(11);
      tooltipUnique->setAutoLayout(tgui::AutoLayout::Bottom);
      tooltipUnique->setScrollbarPolicy(tgui::Scrollbar::Policy::Never);
      tooltipPanel->add(tooltipUnique);

      tooltipPanel->setVisible(false);
    }
  }

  if(window->isVisible() != isVisible)
    window->setVisible(isVisible);
  if(window->getTitle() != name)
    window->setTitle(name); 
  if(window->getPosition() != tgui::Vector2f(pos.x, pos.y))
    window->setPosition(pos.x, pos.y);

  // Update slot contents
  for (u16 y = 0; y < invComp->rows; ++y) {
    for (u16 x = 0; x < invComp->cols; ++x) {
      size_t index = y * invComp->cols + x;
      const ItemStackComponent& slot = invComp->getSlot(x, y);

      auto bg = slotBackgrounds[index];
      auto picture = slotPictures[index];
      auto label = slotLabels[index];

      if (slot.itemId == 0) {
        // Empty slot
        picture->getRenderer()->setTexture(tgui::Texture());
        label->setText("");

        tgui::Color curColor = bg->getRenderer()->getBackgroundColor();
        if(curColor != emptySlotColor && curColor != hoverSlotColor && curColor != dragHoverSlotColor) {
          bg->getRenderer()->setBackgroundColor(emptySlotColor);
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
    if (!isPointInWindowBounds(mousePosVec)) {
      // Mouse released outside inventory - drop the item to the world
      dropItemToWorld(inventoryEntity);
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
    dragSourceView = this;
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

u32 InventoryViewComponent::calculateVisualChecksum(u32 sizeX, u32 sizeY) const {
  u32 hash = 0;
  
  // Hash numeric properties
  hash = ((hash << 5) + hash) ^ std::hash<float>()(padding.x);
  hash = ((hash << 5) + hash) ^ std::hash<float>()(padding.y);
  hash = ((hash << 5) + hash) ^ std::hash<float>()(slotSize);
  hash = ((hash << 5) + hash) ^ std::hash<float>()(rounding);
  
  // Hash color values (RGBA)
  auto hashColor = [](u32& h, const tgui::Color& color) {
    h = ((h << 5) + h) ^ (static_cast<u32>(color.getRed()) << 24);
    h = ((h << 5) + h) ^ (static_cast<u32>(color.getGreen()) << 16);
    h = ((h << 5) + h) ^ (static_cast<u32>(color.getBlue()) << 8);
    h = ((h << 5) + h) ^ static_cast<u32>(color.getAlpha());
  };
  
  hashColor(hash, windowBackgroundColor);
  hashColor(hash, borderColor);
  hashColor(hash, emptySlotColor);
  hashColor(hash, textColor);
  hashColor(hash, hoverSlotColor);
  hashColor(hash, dragHoverSlotColor);
  
  // Hash cached grid dimensions (indicates inventory size)
  hash = ((hash << 5) + hash) ^ sizeX;
  hash = ((hash << 5) + hash) ^ sizeY;
  
  return hash;
}

bool InventoryViewComponent::hasVisualConfigChanged(u32 sizeX, u32 sizeY, u32 previousChecksum) const {
  return calculateVisualChecksum(sizeX, sizeY) != previousChecksum;
}


void InventoryViewComponent::showTooltip(EntityPtr inventoryEntity, size_t slotIndex) {
  if (!inventoryEntity || !inventoryEntity.has<InventoryComponent>() || !tooltipPanel) {
    return;
  }

  auto invComp = inventoryEntity.get<InventoryComponent>();
  if (slotIndex >= invComp->items.size()) {
    return;
  }

  const ItemStackComponent& slot = invComp->items[slotIndex];
  if (slot.itemId == 0) {
    tooltipPanel->setVisible(false);
    return;
  }

  IWorldPtr world = inventoryEntity.world();
  auto itemEntity = world->getEntity(slot.itemId);
  if (!itemEntity || !itemEntity.has<ItemComponent>()) {
    tooltipPanel->setVisible(false);
    return;
  }

  auto itemComp = itemEntity.get<ItemComponent>();

  // Update tooltip content
  tooltipIcon->getRenderer()->setTexture(itemComp->icon);
  tooltipName->setText(itemComp->name);
  tooltipDescription->setText(itemComp->description.empty() ? "No description" : itemComp->description);
  tooltipUnique->setText(itemComp->isUnique ? "Unique Item" : "Stackable");

  // Position tooltip below the inventory window
  if (window) {
    tgui::Vector2f windowPos = window->getPosition();
    tgui::Vector2f windowSize = window->getSize();
    tooltipPanel->setPosition(windowPos.x, windowPos.y + windowSize.y + 10);
  }

  tooltipPanel->setVisible(true);
}

void InventoryViewComponent::hideTooltip() {
  if (tooltipPanel) {
    tooltipPanel->setVisible(false);
  }
}

bool InventoryViewComponent::isPointInWindowBounds(const glm::vec2& worldPos) const {
  if (!window) {
    return false;
  }

  tgui::Vector2f windowPos = window->getPosition();
  tgui::Vector2f windowSize = window->getSize();

  return worldPos.x >= windowPos.x && worldPos.x <= windowPos.x + windowSize.x &&
         worldPos.y >= windowPos.y && worldPos.y <= windowPos.y + windowSize.y;
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