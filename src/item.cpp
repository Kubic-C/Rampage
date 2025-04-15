#include "item.hpp"

Inventory ItemManager::createInventory(std::string name, u8 rows, u8 cols) {
  InventoryId invId = m_idMgr.generate();

  m_inventories.emplace(std::pair(invId, InventoryData()));

  const float sideLength = 64.0f;

  InventoryData& inv = m_inventories.find(invId)->second;
  inv.window = tgui::ChildWindow::create(name + "[" + std::to_string(invId) + "]");
  tgui::Vector2f windowPadding = inv.window->getSize() - inv.window->getClientSize();
  inv.window->setSize(inv.rows * sideLength + windowPadding.x, inv.cols * sideLength + windowPadding.y);
  inv.window->setResizable(false);

  tgui::Grid::Ptr grid = tgui::Grid::create();
  inv.window->add(grid);
  for(int i = 0; i < inv.rows; i++)
    for (int j = 0; j < inv.rows; j++) {
      tgui::BitmapButton::Ptr ui = tgui::BitmapButton::create();
      ui->setSize(sideLength, sideLength);
      ui->setImage(m_defaultTexture);
      grid->add(ui);
      grid->setWidgetCell(ui, i, j);
      inv.items[{i, j}].ui = ui;

      ui->onMousePress([this, invId, i, j]() {
          if (m_handInvId != 0 && !(m_handInvId != invId && m_handInvPos != glm::u16vec2(i, j)))
            getInventory(invId).swapItems({ i, j }, m_handInvId, m_handInvPos);

          logGeneric("grabby\n");
          setHand(invId, { i, j }); 
        });

      ui->onMouseRelease([this, invId, i, j]() {
        if (m_handInvId != 0 && !(m_handInvId != invId && m_handInvPos != glm::u16vec2(i, j)))
          getInventory(invId).swapItems({ i, j }, m_handInvId, m_handInvPos);

          clearHand();
        });
    }

  return getInventory(invId);
}

Inventory ItemManager::getInventory(InventoryId id) {
  return Inventory(*this, id);
}

void ItemManager::destroyInventory(InventoryId id) {
  for (auto& [pos, entity] : m_inventories.find(id)->second.items) {
    Entity e = m_world.get(entity.item);
    if (e.has<ItemAttrUnique>())
      m_world.destroy(e);
  }
}