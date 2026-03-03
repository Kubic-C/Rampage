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
}

void ItemComponent::deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component) {
  auto itemReader = reader.getRoot<Schema::ItemComponent>();
  auto self = component.cast<ItemComponent>();

  self->name = itemReader.getName();
  self->description = itemReader.getDescription();
  self->maxStackSize = itemReader.getMaxStackSize();
  self->stackCost = itemReader.getStackCost();
  self->isUnique = itemReader.getIsUnique();
  // TODO: Handle TGUI's textures.
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
  auto itemsBuilder = invBuilder.initItems(self->items.size());
  for (size_t i = 0; i < self->items.size(); ++i) {
    itemsBuilder[i].setCount(self->items[i].count);
    itemsBuilder[i].setItemId(self->items[i].itemId);
  }
}

void InventoryComponent::deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component) {
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
    stack.itemId = id.resolve(itemReader.getItemId()); // Remap old EntityId -> new EntityId
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

void InventoryViewComponent::update(IWorldPtr world, tgui::Gui& gui) {
  // Get the inventory entity and component
  auto invEntity = world->getEntity(inventoryEntityId);
  if (!invEntity || !invEntity.has<InventoryComponent>()) {
    return; // Inventory doesn't exist or doesn't have component
  }

  auto invComp = invEntity.get<InventoryComponent>();

  // Check if inventory grid size changed (rebuild UI if it did)
  bool gridSizeChanged = false;
  if (cachedRows != invComp->rows || cachedCols != invComp->cols) {
    gridSizeChanged = true;
    cachedRows = invComp->rows;
    cachedCols = invComp->cols;
    
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
        label->onMousePress([this, slotPos, world]() {
          if (isInteractable) {
            onSlotMouseDown(world, slotPos);
          }
        });
        
        label->onMouseRelease([this, slotPos, world]() {
          if (isInteractable) {
            onSlotMouseUp(world, slotPos);
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
  }

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

  // Update visibility
  window->setVisible(isVisible);
}

void InventoryViewComponent::onSlotMouseDown(IWorldPtr world, glm::u16vec2 slotPos) {
  // Check if slot is empty - only start drag if there's an item
  auto invEntity = world->getEntity(inventoryEntityId);
  if (!invEntity || !invEntity.has<InventoryComponent>()) {
    return;
  }

  auto invComp = invEntity.get<InventoryComponent>();
  const ItemStack& slot = invComp->getSlot(slotPos.x, slotPos.y);

  if (slot.itemId != 0) {
    isDragging = true;
    dragStartSlot = slotPos;
    dragSourceView = this;
  }
}

void InventoryViewComponent::onSlotMouseUp(IWorldPtr world, glm::u16vec2 dropSlot) {
  if (!isDragging) {
    return;
  }

  isDragging = false;

  // Only perform drop if we have a valid source view
  if (dragSourceView != nullptr) {
    performItemDrop(world, dragSourceView, dragStartSlot, dropSlot);
  }

  dragSourceView = nullptr;
}

void InventoryViewComponent::performItemDrop(IWorldPtr world, InventoryViewComponent* sourceView, 
                                             glm::u16vec2 sourceSlot, glm::u16vec2 targetSlot) {
  // Use InventoryManager to perform the move
  InventoryManager invMgr;

  // Source and target inventory entity IDs
  EntityId srcInvId = sourceView->inventoryEntityId;
  EntityId dstInvId = inventoryEntityId;

  // Perform the move operation
  // moveItems intelligently merges stacks or swaps based on item types
  invMgr.moveItems(world, srcInvId, sourceSlot.x, sourceSlot.y,
                   dstInvId, targetSlot.x, targetSlot.y, 0); // 0 = move all
}


RAMPAGE_END