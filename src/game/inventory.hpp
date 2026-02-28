#pragma once

#include "components/components.hpp"

RAMPAGE_START

struct ItemStack {
  u32 maxStackCost = 64;
  i32 stackCount = 0;
  EntityId item = 0;
  tgui::BitmapButton::Ptr ui; // do not erase

  bool doesExceedMaxStackCost(u8 stackCost, i32 additional) {
    return (stackCount + additional) * stackCost > maxStackCost;
  }

  void reset(const tgui::Texture& defaultTexture) {
    stackCount = 0;
    item = 0;
    ui->setImage(defaultTexture);
    ui->setText("");
  }
};

struct InventoryData {
  std::string name;
  u16 rows = 3;
  u16 cols = 3;
  OpenMap<glm::u16vec2, ItemStack> items;
  tgui::ChildWindow::Ptr window;
  bool m_visible = false;
};

class Inventory;

class InventoryManager {
  friend class Inventory;

public:
  InventoryManager(IWorldPtr world) : m_world(world) {
    tgui::Gui& gui = m_world->getContext<tgui::Gui>();
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

  Inventory createInventory(std::string name, u8 rows = 3, u8 cols = 3);
  Inventory getInventory(InventoryId id);
  bool hasInventory(InventoryId id);
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

    tgui::Gui& gui = m_world->getContext<tgui::Gui>();
    gui.setOverrideMouseCursor(tgui::Cursor::Type::Hand);

    EntityId eId = m_inventories.find(invId)->second.items.find(pos)->second.item;
    if (eId) {
      EntityPtr e = getEntity(eId);
      m_handPicture->getRenderer()->setTexture(e.get<ItemAttrIcon>()->icon);
      m_handPicture->setVisible(true);
    }
  }

  void clearHand() {
    m_handInvId = 0;
    m_handInvPos = {0, 0};

    tgui::Gui& gui = m_world->getContext<tgui::Gui>();
    gui.setOverrideMouseCursor(tgui::Cursor::Type::Arrow);

    m_handPicture->setVisible(false);
  }

  bool isHandEmpty() const {
    return m_handInvId == 0;
  }

  Inventory getHandInventory();

  glm::u16vec2 getHandInventoryPos() {
    return m_handInvPos;
  }

protected:
  InventoryData& getInventoryData(InventoryId id) {
    return m_inventories.find(id)->second;
  }

  EntityPtr getEntity(EntityId entity) {
    return m_world->getEntity(entity);
  }

private:
  IWorldPtr m_world;

  tgui::Texture m_defaultTexture;

  IdManager<InventoryId> m_idMgr;
  Map<InventoryId, InventoryData> m_inventories;

  InventoryId m_handInvId = 0;
  glm::u16vec2 m_handInvPos = {0, 0};
  tgui::Picture::Ptr m_handPicture;
};

/* A proxy class used to extend functionality */
class Inventory {
public:
  Inventory(InventoryManager& mgr, InventoryId id) : m_mgr(mgr), m_id(id) {}

  bool addItem(EntityId entity, const glm::u16vec2& pos, u32 count = 1) {
    InventoryData& inv = getData();
    EntityPtr itemEntity = m_mgr.getEntity(entity);
    u8 stackCost = itemEntity.get<ItemAttrStackCost>()->stackCost;

    if (!inv.items.contains(pos))
      return false;

    ItemStack& stack = inv.items.find(pos)->second;
    // If the item is unique, and the itemStack is not empty, return false
    // If the itemStack does contain the same entity, return false
    // If the added stackCost would exceed the maxStackCost, return false
    if (itemEntity.has<ItemAttrUnique>() && stack.item != 0 || (entity != stack.item && stack.item != 0) ||
        (stack.stackCount + count) * stackCost > stack.maxStackCost)
      return false;

    stack.stackCount += count;
    if (itemEntity.has<ItemAttrUnique>() || stack.item == 0) {
      stack.item = entity;
      stack.ui->setImage(itemEntity.get<ItemAttrIcon>()->icon);
    }

    if (!itemEntity.has<ItemAttrUnique>())
      stack.ui->setText(std::to_string(stack.stackCount));

    return true;
  }

  bool addItem(EntityId entity, u32 count = 1) {
    InventoryData& inv = getData();
    EntityPtr itemEntity = m_mgr.getEntity(entity);
    u8 stackCost = itemEntity.get<ItemAttrStackCost>()->stackCost;

    // If the item is unique, it can only contain one slot.
    if (itemEntity.has<ItemAttrUnique>()) {
      for (u16 y = 0; y < inv.rows; y++) {
        for (u16 x = 0; x < inv.cols; x++) {
          if (count == 0)
            return true;

          ItemStack& stack = inv.items.find({x, y})->second;
          if (stack.item == 0) {
            stack.item = itemEntity.clone();
            stack.stackCount = 1;
            stack.ui->setImage(itemEntity.get<ItemAttrIcon>()->icon);
            count--;
          }
        }
      }
    } else {
      // Search existing stacks to see if any can fit this item
      for (u16 y = 0; y < inv.rows; y++) {
        for (u16 x = 0; x < inv.cols; x++) {
          ItemStack& stack = inv.items.find({x, y})->second;
          if (stack.doesExceedMaxStackCost(stackCost, count))
            continue;
          if (stack.item == itemEntity) {
            stack.stackCount += count;
            stack.ui->setText(std::to_string(stack.stackCount));
            return true;
          }
        }
      }

      // Now find the first empty one
      for (u16 y = 0; y < inv.rows; y++) {
        for (u16 x = 0; x < inv.cols; x++) {
          ItemStack& stack = inv.items.find({x, y})->second;
          if (stack.doesExceedMaxStackCost(stackCost, count))
            continue;
          if (stack.item == 0) {
            stack.item = entity;
            stack.stackCount = count;
            stack.ui->setImage(itemEntity.get<ItemAttrIcon>()->icon);
            stack.ui->setText(std::to_string(stack.stackCount));
            return true;
          }
        }
      }
    }

    return false; // Inventory is full
  }

  const ItemStack getStack(const glm::u16vec2& pos) const {
    return getData().items.find(pos)->second;
  }

  bool isStackEmpty(const glm::u16vec2& pos) {
    return getData().items.find(pos)->second.item == 0;
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
      EntityPtr aItem = m_mgr.getEntity(a.item);
      aCost = aItem.get<ItemAttrStackCost>()->stackCost;
      aItemUnique = aItem.has<ItemAttrUnique>();
      aIcon = aItem.get<ItemAttrIcon>()->icon;
    }
    u8 bCost = 0;
    tgui::Texture bIcon = m_mgr.m_defaultTexture;
    bool bItemUnique = false;
    if (b.item != 0) {
      EntityPtr bItem = m_mgr.getEntity(b.item);
      bCost = bItem.get<ItemAttrStackCost>()->stackCost;
      bItemUnique = bItem.has<ItemAttrUnique>();
      bIcon = bItem.get<ItemAttrIcon>()->icon;
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

  bool tryMergeSlots(const glm::u16vec2& posA, InventoryId invIdB, const glm::u16vec2& posB) {
    ItemStack& aStack = getData().items.find(posA)->second;
    ItemStack& bStack = m_mgr.getInventoryData(invIdB).items.find(posB)->second;

    if (aStack.item != bStack.item || m_mgr.getEntity(aStack.item).has<ItemAttrUnique>())
      return false;

    EntityPtr item = m_mgr.getEntity(aStack.item);
    u8 stackCost = item.get<ItemAttrStackCost>()->stackCost;
    i32 aCurrentStackCost = stackCost * aStack.stackCount;
    i32 bCurrentStackCost = stackCost * bStack.stackCount;
    i32 combinedCost = aCurrentStackCost + bCurrentStackCost;

    if (combinedCost <= bStack.maxStackCost) {
      bStack.stackCount += aStack.stackCount; // order matters here
      removeItem(posA, aStack.stackCount);
      bStack.ui->setText(std::to_string(bStack.stackCount));
      return true;
    }

    i32 overCount = (combinedCost - bStack.maxStackCost) / stackCost;

    aStack.stackCount = overCount;
    bStack.stackCount = bStack.maxStackCost / stackCost;

    aStack.ui->setText(std::to_string(aStack.stackCount));
    bStack.ui->setText(std::to_string(bStack.stackCount));

    return false;
  }

  EntityPtr removeItem(const glm::u16vec2& pos, u32 count = 1) {
    InventoryData& inv = getData();
    ItemStack& stack = inv.items.find(pos)->second;
    if (!stack.item)
      return EntityPtr(m_mgr.m_world, 0);

    EntityPtr stackEntity = m_mgr.getEntity(stack.item);
    if (stackEntity.has<ItemAttrUnique>()) {
      stack.reset(m_mgr.m_defaultTexture);
      return stackEntity;
    } else {
      stack.stackCount -= count;
      if (stack.stackCount == 0)
        stack.reset(m_mgr.m_defaultTexture);
      else
        stack.ui->setText(std::to_string(stack.stackCount));
      return stackEntity;
    }
  }

  void setVisible(bool visiblity) {
    InventoryData& inv = getData();
    if (visiblity == inv.m_visible)
      return;
    inv.m_visible = visiblity;

    tgui::Gui& gui = m_mgr.m_world->getContext<tgui::Gui>();
    if (inv.m_visible)
      gui.add(inv.window);
    else
      gui.remove(inv.window);
  }

  bool getVisible() const {
    return getData().m_visible;
  }

  operator InventoryId() const {
    return m_id;
  }

protected:
  InventoryData& getData() {
    return m_mgr.getInventoryData(m_id);
  }

  const InventoryData& getData() const {
    return m_mgr.getInventoryData(m_id);
  }

private:
  InventoryManager& m_mgr;
  InventoryId m_id;
};

inline bool tryPlaceItem(EntityPtr worldMap, Inventory inv, const glm::u16vec2& stackPos,
                         const glm::vec2& coords) {
  IWorldPtr world = worldMap.world();
  AssetLoader& assetLoader = world->getContext<AssetLoader>();

  const ItemStack stack = inv.getStack(stackPos);
  if (stack.item == 0 || !world->getEntity(stack.item).has<ItemAttrTile>())
    return false;

  EntityPtr item = world->getEntity(stack.item);
  RefT<TilemapComponent> tmLayers = worldMap.get<TilemapComponent>();
  Tilemap& topTilemap = tmLayers->getToptilemap();
  Tilemap& bottomTilemap = tmLayers->getBottomtilemap();

  RefT<BodyComponent> body = worldMap.get<BodyComponent>();
  glm::i16vec2 tilePos = Tilemap::getNearestTile(worldMap.get<TransformComponent>()->getLocalPoint(coords));
  const TileDef& tileItemPrefab = assetLoader.getTilePrefab(item.get<ItemAttrTile>()->tileId);
  for (int x = tilePos.x; x < tilePos.x + tileItemPrefab.width(); x++) {
    for (int y = tilePos.y; y < tilePos.y + tileItemPrefab.height(); y++) {
      glm::i16vec2 tilePos = {x, y};

      if (!bottomTilemap.contains(tilePos))
        continue;
      bool isCollidable = bottomTilemap.find(tilePos).flags & TileFlags::IS_COLLIDABLE;
      if (isCollidable)
        return false;
    }
  }

  for (int x = tilePos.x; x < tilePos.x + tileItemPrefab.width(); x++) {
    for (int y = tilePos.y; y < tilePos.y + tileItemPrefab.height(); y++) {
      glm::i16vec2 tilePos = {x, y};

      if (!topTilemap.contains(tilePos))
        continue;

      EntityPtr tile = world->getEntity(topTilemap.erase(tilePos));
      if (tile.has<TileItemComponent>())
        inv.addItem(tile.get<TileItemComponent>()->item);
      world->destroy(tile);
    }
  }

  inv.removeItem(stackPos);
  tmLayers->getToptilemap().insert(world, body->id, tilePos, worldMap,
                                   assetLoader.cloneTilePrefab(item.get<ItemAttrTile>()->tileId));

  if (inv.isStackEmpty(stackPos))
    world->getContext<InventoryManager>().clearHand();

  return true;
}

RAMPAGE_END
