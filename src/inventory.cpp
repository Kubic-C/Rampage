#include "inventory.hpp"

Inventory InventoryManager::createInventory(std::string name, u8 rows, u8 cols) {
  InventoryId invId = m_idMgr.generate();

  m_inventories.emplace(std::pair(invId, InventoryData()));

  const float sideLength = 64.0f;

  InventoryData& inv = m_inventories.find(invId)->second;
  inv.rows = rows;
  inv.cols = cols;
  inv.window = tgui::ChildWindow::create(name + "[" + std::to_string(invId) + "]");
  tgui::Vector2f windowPadding = inv.window->getSize() - inv.window->getClientSize();
  inv.window->setSize(inv.cols * sideLength + windowPadding.x, inv.rows * sideLength + windowPadding.y);
  inv.window->setResizable(false);

  tgui::Grid::Ptr grid = tgui::Grid::create();
  inv.window->add(grid);
  for(int i = 0; i < inv.cols; i++)
    for (int j = 0; j < inv.rows; j++) {
      tgui::BitmapButton::Ptr ui = tgui::BitmapButton::create();
      ui->setSize(sideLength, sideLength);
      ui->setImage(m_defaultTexture);
      grid->add(ui);
      grid->setWidgetCell(ui, j, i); // setWidgetCell takes its params like (row, col), so swap and i and j
      inv.items[{i, j}].ui = ui;

      ui->onMousePress([this, invId, i, j]() {
          if (getInventory(invId).isStackEmpty({ i, j }))
            return;

          if (m_handInvId != 0 && !(m_handInvId != invId && m_handInvPos != glm::u16vec2(i, j)))
            getInventory(invId).swapItems({ i, j }, m_handInvId, m_handInvPos);

          setHand(invId, { i, j }); 
        });

      ui->onMouseRelease([this, invId, i, j]() {
        if (m_handInvId != 0 && !(m_handInvId == invId && m_handInvPos == glm::u16vec2(i, j))) {
          if (!getInventory(m_handInvId).tryMergeSlots(m_handInvPos, invId, {i, j})) {
            getInventory(invId).swapItems({ i, j }, m_handInvId, m_handInvPos);
          }
        }

          clearHand();
        });
    }

  return getInventory(invId);
}

Inventory InventoryManager::getHandInventory() {
  return Inventory(*this, m_handInvId);
}

Inventory InventoryManager::getInventory(InventoryId id) {
  return Inventory(*this, id);
}

bool InventoryManager::hasInventory(InventoryId id) {
  return m_inventories.contains(id);
}

void InventoryManager::destroyInventory(InventoryId id) {
  for (auto& [pos, stack] : m_inventories.find(id)->second.items) {
    if (stack.item == 0)
      continue;

    Entity e = m_world.get(stack.item);
    if (e.has<ItemAttrUnique>())
      m_world.destroy(e);
  }

  InventoryData& inv = getInventoryData(id);
  inv.window->destroy();
  m_inventories.erase(id);
  m_idMgr.destroy(id);
}