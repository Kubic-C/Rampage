#pragma once

#include "../inventory.hpp"
#include "../components/item.hpp"

class ItemModule : public Module {
public:
  static void registerComponents(EntityWorld& world) {
    world.component<ItemAttrStackCost>();
    world.component<ItemAttrUnique>();
    world.component<ItemAttrIcon>();
    world.component<ItemAttrTile>();
    world.component<TileItemComponent>();

    world.observe(EntityWorld::EventType::Remove, world.component<InventoryComponent>(), {},
      [](Entity e) {
        InventoryManager& mgr = e.world().getContext<InventoryManager>();
        if (mgr.hasInventory(e.get<InventoryComponent>()->id))
          mgr.destroyInventory(e.get<InventoryComponent>()->id);
      });
  }

  ItemModule(EntityWorld& world)
    : m_world(world) {}

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