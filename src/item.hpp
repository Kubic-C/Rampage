#pragma once

#include "utility/ecs.hpp"
#include "utility/log.hpp"

using ItemId = u32;
using InventoryId = u32;

template<>
struct boost::hash<glm::u16vec2> {
  size_t operator()(const glm::u16vec2& pos) const {
    constexpr u16 prime1 = 65521;
    constexpr u16 prime2 = 57149;

    return (pos.x ^ prime1 * 0x5555) ^ (pos.y ^ prime2 * 0x5555);
  }
};

struct InventoryComponent {
  InventoryId id;
};

struct ItemAttrStackCost {
  u8 stackCost = 1;
};

struct ItemAttrIcon {
  tgui::Texture icon;
};

struct ItemAttrUnique {};

struct ItemStack {
  u32 maxStackCost = 64;
  i32 stackCount = 0;
  EntityId item = 0;
  tgui::BitmapButton::Ptr ui; // do not erase

  void reset(const tgui::Texture& defaultTexture) {
    stackCount = 0;
    item = 0;
    ui->setImage(defaultTexture);
  }
};

struct InventoryData {
  std::string name;
  u16 rows = 3;
  u16 cols = 3;
  OpenMap<glm::u16vec2, ItemStack> items;
  tgui::ChildWindow::Ptr window;
};

class Inventory;

class ItemManager {
  friend class Inventory;

public:
  ItemManager(EntityWorld& world)
    : m_world(world) {
    tgui::Gui& gui = m_world.getContext<tgui::Gui>();
    m_handPicture = tgui::Picture::create();
    m_handPicture->setEnabled(false);
    m_handPicture->setVisible(false); // added for emphasis
    m_handPicture->ignoreMouseEvents(true);
    gui.add(m_handPicture);
  }

  void setDefaultItemIcon(tgui::Texture texture) {
    m_defaultTexture = texture;
    m_handPicture->getRenderer()->setTexture(m_defaultTexture);
  }

  ItemId createItem(const std::string& name, Entity entity, tgui::Texture icon, u8 stackCost = 1) {
    ItemId id = m_idMgr.generate();
    m_itemNames[name] = id;

    entity.disable();
    entity.add<ItemAttrStackCost>();
    ItemAttrStackCost& itemStackCost = entity.get<ItemAttrStackCost>();
    itemStackCost.stackCost = stackCost;
    entity.add<ItemAttrIcon>();
    ItemAttrIcon& itemIcon = entity.get<ItemAttrIcon>();
    itemIcon.icon = icon;
    m_items[id] = entity;

    return id;
  }

  Entity getItem(ItemId id) const {
    Entity e = m_world.get(m_items.at(id));

    return e;
  }

  Entity getItem(const std::string& name) const {
    assert(m_itemNames.contains(name));
    return getItem(m_itemNames.at(name));
  }

  Inventory createInventory(std::string name, u8 rows = 3, u8 cols = 3);
  Inventory getInventory(InventoryId id);
  void destroyInventory(InventoryId id);

  void updateHandPos() {
    float x, y;
    SDL_GetMouseState(&x, &y);
  
    m_handPicture->setPosition(x, y);
    m_handPicture->moveToFront();
  }

  void setHand(InventoryId invId, const glm::u16vec2& pos) {
    m_handInvId = invId;
    m_handInvPos = pos;

    tgui::Gui& gui = m_world.getContext<tgui::Gui>();
    gui.setOverrideMouseCursor(tgui::Cursor::Type::Hand);

    EntityId eId = m_inventories.find(invId)->second.items.find(pos)->second.item;
    if (eId) {
      Entity e = getEntity(eId);
      m_handPicture->getRenderer()->setTexture(e.get<ItemAttrIcon>().icon);
      m_handPicture->setVisible(true);
    }
  }

  void clearHand() {
    m_handInvId = 0;
    m_handInvPos = { 0, 0 };

    tgui::Gui& gui = m_world.getContext<tgui::Gui>();
    gui.setOverrideMouseCursor(tgui::Cursor::Type::Arrow);

    m_handPicture->setVisible(false);
  }

  InventoryId getHandInventory() {
    return m_handInvId;
  }

  glm::u16vec2 getHandInventoryPos() {
    return m_handInvPos;
  }

protected:
  InventoryData& getInventoryData(InventoryId id) {
    return m_inventories.find(id)->second;
  }

  Entity getEntity(EntityId entity) {
    return m_world.get(entity);
  }

private:
  tgui::Texture m_defaultTexture;

  EntityWorld& m_world;
  IdManager<ItemId> m_idMgr;

  Map<ItemId, EntityId> m_items;
  Map<std::string, ItemId> m_itemNames;

  Map<InventoryId, InventoryData> m_inventories;

  InventoryId m_handInvId = 0;
  glm::u16vec2 m_handInvPos = { 0, 0 };
  tgui::Picture::Ptr m_handPicture;
};

/* A proxy class used to extend functionality */
class Inventory {
public:
  Inventory(ItemManager& mgr, InventoryId id) 
    : m_mgr(mgr), m_id(id) {}

  bool addItem(EntityId entity, u32 count = 1) {
    InventoryData& inv = getData();
    Entity itemEntity = m_mgr.getEntity(entity);
    u8 stackCost = itemEntity.get<ItemAttrStackCost>().stackCost;
  
    // If the item is unique, it can only contain one slot.
    if (itemEntity.has<ItemAttrUnique>()) {
      if (count > 1) // Unique items do not exist in multiples
        return false;

      for (u16 x = 0; x < inv.rows; x++) {
        for (u16 y = 0; y < inv.cols; y++) {
          ItemStack& stack = inv.items.find({ x, y })->second;
          if (stack.item == 0) {
            stack.item = entity;
            stack.stackCount = 1;
            stack.ui->setImage(itemEntity.get<ItemAttrIcon>().icon);
            return true;
          }
        }
      }
    }

    // Search existing stacks to see if any can fit this item, or find the first empty one
    for (auto& [pos, stack] : inv.items) {
      // Even if the entities are not the same, this condition must be checked
      // for both conditionals below, so were essentially killing 2 birds with one stone.
      if ((stack.stackCount + count) * stackCost > stack.maxStackCost)
        continue;

      if (stack.item == itemEntity) {
        stack.stackCount += count;
        stack.ui->setText(std::to_string(stack.stackCount));
        return true;
      }
      else if (stack.item == 0) {
        stack.item = entity;
        stack.stackCount = count;
        stack.ui->setImage(itemEntity.get<ItemAttrIcon>().icon);
        stack.ui->setText(std::to_string(stack.stackCount));
        return true;
      }
    }

    return false; // Inventory is full
  }

  bool swapItems(const glm::u16vec2& posA, InventoryId invIdB, const glm::u16vec2& posB) {
    // its not as bad as it looks i swear

    InventoryData& invA = getData();
    InventoryData& invB = m_mgr.getInventoryData(invIdB);
    ItemStack& a = invA.items.find(posA)->second;
    ItemStack& b = invB.items.find(posB)->second;
    if (a.item == 0 && b.item == 0)
      return true;

    u8 aCost = 0;
    tgui::Texture aIcon = m_mgr.m_defaultTexture;
    bool aItemUnique = false;
    if (a.item != 0) {
      Entity aItem = m_mgr.getEntity(a.item);
      aCost = aItem.get<ItemAttrStackCost>().stackCost;
      aItemUnique = aItem.has<ItemAttrUnique>();
      aIcon = aItem.get<ItemAttrIcon>().icon;
    }
    u8 bCost = 0;
    tgui::Texture bIcon = m_mgr.m_defaultTexture;
    bool bItemUnique = false;
    if (b.item != 0) {
      Entity bItem = m_mgr.getEntity(b.item);
      bCost = bItem.get<ItemAttrStackCost>().stackCost;
      bItemUnique = bItem.has<ItemAttrUnique>();
      bIcon = bItem.get<ItemAttrIcon>().icon;
    }

    if (a.maxStackCost < (b.stackCount * bCost) || b.maxStackCost < (a.stackCount * aCost))
      return false;

    EntityId aCopyItem = a.item;
    i32 aCopyStackCount = a.stackCount;
    
    a.stackCount = b.stackCount;
    a.item = b.item;
    a.ui->setImage(bIcon);
    if (!bItemUnique && a.stackCount != 0)
      a.ui->setText(std::to_string(a.stackCount));
    else
      a.ui->setText("");

    b.stackCount = aCopyStackCount;
    b.item = aCopyItem;
    b.ui->setImage(aIcon);
    if (!aItemUnique && b.stackCount != 0)
      b.ui->setText(std::to_string(b.stackCount));
    else
      b.ui->setText("");

    return true;
  }

  Entity removeItem(const glm::u16vec2& pos) {
    InventoryData& inv = getData();
    ItemStack& stack = inv.items.find(pos)->second;
    if (!stack.item)
      return Entity(m_mgr.m_world, 0);

    Entity stackEntity = m_mgr.getEntity(stack.item);

    if (stackEntity.has<ItemAttrUnique>()) {
      stack.reset(m_mgr.m_defaultTexture);
      return stackEntity;
    } else {
      u8 stackCost = stackEntity.get<ItemAttrStackCost>().stackCost;
      stack.stackCount -= 1;
      if (stack.stackCount == 0)
        stack.reset(m_mgr.m_defaultTexture);
      else
        stack.ui->setText(std::to_string(stack.stackCount));
      return stackEntity;
    }
  }

  void setVisible(bool visiblity) {
    if (visiblity == m_visible)
      return;

    InventoryData& inv = m_mgr.getInventoryData(m_id);
    tgui::Gui& gui = m_mgr.m_world.getContext<tgui::Gui>();

    if (m_visible) {
      gui.remove(inv.window);
    } else {
      gui.add(inv.window);
    }

    m_visible = visiblity;
  }

     
protected:
  InventoryData& getData() {
    return m_mgr.getInventoryData(m_id);
  }

private:
  bool m_visible = false;
  ItemManager& m_mgr;
  InventoryId m_id;
};

class ItemModule : public Module {
public:
  ItemModule(EntityWorld& world)
    : m_world(world) {
    m_world.component<ItemAttrStackCost>();
    m_world.component<ItemAttrUnique>();
    m_world.component<ItemAttrIcon>();
   
    m_world.addContext<ItemManager>(world);
  }

  void run(EntityWorld& world, float deltaTime) override {
    ItemManager& itemMgr = m_world.getContext<ItemManager>();

    itemMgr.updateHandPos();
    if (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_RMASK) {
      static int i = 0;
      itemMgr.clearHand();
    }
  }

private:
  EntityWorld& m_world;
};