#pragma once

#include "../inventory.hpp"
#include "../components/item.hpp"

class ItemModule : public Module {
public:
  ItemModule(EntityWorld& world)
    : m_world(world) {
    m_world.component<ItemAttrStackCost>();
    m_world.component<ItemAttrUnique>();
    m_world.component<ItemAttrIcon>();
    m_world.component<ItemAttrTile>();
    m_world.component<TileItemComponent>();
    m_world.addContext<InventoryManager>(world);

    m_world.observe(EntityWorld::EventType::Remove, m_world.component<InventoryComponent>(), {},
      [](Entity e) {
        InventoryManager& mgr = e.world().getContext<InventoryManager>();
        if (mgr.hasInventory(e.get<InventoryComponent>().id))
          mgr.destroyInventory(e.get<InventoryComponent>().id);
      });
  }

  void run(EntityWorld& world, float deltaTime) override {
    InventoryManager& itemMgr = m_world.getContext<InventoryManager>();

    itemMgr.updateHandPos();
    if (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_RMASK) {
      itemMgr.clearHand();
    }
  }

private:
  EntityWorld& m_world;
};