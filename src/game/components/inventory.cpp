#include "inventory.hpp"
#include "../systems/inventory.hpp"

RAMPAGE_START

// Static member definitions for global drag state
bool InventoryViewComponent::isDragging = false;
glm::u16vec2 InventoryViewComponent::dragStartSlot = {0, 0};
InventoryViewComponent* InventoryViewComponent::dragSourceView = nullptr;

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
  self->icon = tgui::Texture(itemReader.getIconPath().cStr()); // Load texture from path
}

void ItemComponent::fromJson(Ref component, AssetLoader loader, const json& compJson) {
  auto self = component.cast<ItemComponent>();

  if (compJson.contains("name") && compJson["name"].is_string()) {
    self->name = compJson["name"];
  }

  if (compJson.contains("description") && compJson["description"].is_string()) {
    self->description = compJson["description"];
  }

  if (compJson.contains("maxStackSize") && compJson["maxStackSize"].is_number_unsigned()) {
    self->maxStackSize = compJson["maxStackSize"];
  }

  if (compJson.contains("stackCost") && compJson["stackCost"].is_number_unsigned()) {
    self->stackCost = compJson["stackCost"];
  }

  if (compJson.contains("isUnique") && compJson["isUnique"].is_boolean()) {
    self->isUnique = compJson["isUnique"];
  }

  // Load icon from asset loader if iconAsset is specified
  if (compJson.contains("iconAsset") && compJson["iconAsset"].is_string()) {
    std::string iconPath = compJson["iconAsset"];
    self->icon = tgui::Texture(iconPath);
  }
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
    ItemStack stack;
    stack.count = itemReader.getCount();
    stack.itemId = idMapper.resolve(itemReader.getItemId());
    self->items.push_back(stack);
  }
}

void InventoryComponent::fromJson(Ref component, AssetLoader loader, const json& compJson) {
  auto self = component.cast<InventoryComponent>();

  if (compJson.contains("cols") && compJson["cols"].is_number_unsigned()) {
    self->cols = compJson["cols"];
  }

  if (compJson.contains("rows") && compJson["rows"].is_number_unsigned()) {
    self->rows = compJson["rows"];
  }

  // Initialize slots
  self->items.clear();
  self->items.resize(self->cols * self->rows); // Initialize slots

  // Optionally load initial items from JSON (for asset definitions)
  if (compJson.contains("items") && compJson["items"].is_array()) {
    const auto& itemsJson = compJson["items"];
    size_t slotIndex = 0;

    for (const auto& itemJson : itemsJson) {
      if (slotIndex >= self->items.size()) break;
      if (!itemJson.is_object()) continue;

      if (itemJson.contains("count") && itemJson["count"].is_number_unsigned()) {
        self->items[slotIndex].count = itemJson["count"];
      }

      if (itemJson.contains("itemEntityName") && itemJson["itemEntityName"].is_string()) {
        // Resolve entity by name from asset loader
        std::string entityName = itemJson["itemEntityName"];
        self->items[slotIndex].itemId = loader.getAsset(entityName);
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

void InventoryViewComponent::fromJson(Ref component, AssetLoader loader, const json& compJson) {
  auto self = component.cast<InventoryViewComponent>();

  if (compJson.contains("name") && compJson["name"].is_string()) {
    self->name = compJson["name"];
  }

  if (compJson.contains("pos") && compJson["pos"].is_object()) {
    const auto& posJson = compJson["pos"];
    if (posJson.contains("x") && posJson["x"].is_number()) {
      self->pos.x = posJson["x"];
    }
    if (posJson.contains("y") && posJson["y"].is_number()) {
      self->pos.y = posJson["y"];
    }
  }

  if (compJson.contains("isVisible") && compJson["isVisible"].is_boolean()) {
    self->isVisible = compJson["isVisible"];
  }

  if (compJson.contains("isInteractable") && compJson["isInteractable"].is_boolean()) {
    self->isInteractable = compJson["isInteractable"];
  }

  if (compJson.contains("padding") && compJson["padding"].is_object()) {
    const auto& padJson = compJson["padding"];
    if (padJson.contains("x") && padJson["x"].is_number()) {
      self->padding.x = padJson["x"];
    }
    if (padJson.contains("y") && padJson["y"].is_number()) {
      self->padding.y = padJson["y"];
    }
  }

  if (compJson.contains("slotSize") && compJson["slotSize"].is_number()) {
    self->slotSize = compJson["slotSize"];
  }

  if (compJson.contains("rounding") && compJson["rounding"].is_number()) {
    self->rounding = compJson["rounding"];
  }

  // Helper lambda to parse color from JSON (expecting array [R,G,B,A] or hex string)
  auto parseColor = [](const json& colorJson) -> tgui::Color {
    if (colorJson.is_array() && colorJson.size() == 4) {
      return tgui::Color(
        (colorJson[0].is_number() ? (int)colorJson[0] : (int)0),
        (colorJson[1].is_number() ? (int)colorJson[1] : (int)0),
        (colorJson[2].is_number() ? (int)colorJson[2] : (int)0),
        (colorJson[3].is_number() ? (int)colorJson[3] : (int)255)
      );
    }
    return tgui::Color::Black;
  };

  if (compJson.contains("windowBackgroundColor")) {
    self->windowBackgroundColor = parseColor(compJson["windowBackgroundColor"]);
  }

  if (compJson.contains("borderColor")) {
    self->borderColor = parseColor(compJson["borderColor"]);
  }

  if (compJson.contains("emptySlotColor")) {
    self->emptySlotColor = parseColor(compJson["emptySlotColor"]);
  }

  if (compJson.contains("textColor")) {
    self->textColor = parseColor(compJson["textColor"]);
  }

  if (compJson.contains("hoverSlotColor")) {
    self->hoverSlotColor = parseColor(compJson["hoverSlotColor"]);
  }

  if (compJson.contains("dragHoverSlotColor")) {
    self->dragHoverSlotColor = parseColor(compJson["dragHoverSlotColor"]);
  }
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

        label->onMouseEnter([this, index]() {
          if (isInteractable) {
            slotBackgrounds[index]->getRenderer()->setBackgroundColor(isDragging ? dragHoverSlotColor : hoverSlotColor);
          }
        });

        label->onMouseLeave([this, index]() {
          if (isInteractable) {
            slotBackgrounds[index]->getRenderer()->setBackgroundColor(emptySlotColor);
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
      const ItemStack& slot = invComp->getSlot(x, y);

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
}

void InventoryViewComponent::onSlotMouseDown(EntityPtr inventoryEntity, glm::u16vec2 slotPos) {
  // Check if slot is empty - only start drag if there's an item
  if (!inventoryEntity || !inventoryEntity.has<InventoryComponent>()) {
    return;
  }

  auto invComp = inventoryEntity.get<InventoryComponent>();
  const ItemStack& slot = invComp->getSlot(slotPos.x, slotPos.y);

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


RAMPAGE_END