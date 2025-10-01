#include "item.hpp"

#include "../components/item.hpp"
#include "../inventory.hpp"
#include "../module.hpp"

RAMPAGE_START

int updateItemHands(EntityWorld& world, float deltaTime) {
  InventoryManager& itemMgr = world.getContext<InventoryManager>();

  itemMgr.updateHandPos();
  if (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_RMASK) { // TODO: Switch out to EventModule
    itemMgr.clearHand();
  }

  return 0;
}

void destroyInventoryOnEntityDestruction(Entity e) {
  InventoryManager& mgr = e.world().getContext<InventoryManager>();
  if (mgr.hasInventory(e.get<InventoryComponent>()->id))
    mgr.destroyInventory(e.get<InventoryComponent>()->id);
}

void loadItemSystems(IHost& host) {
  Pipeline& pipeline = host.getPipeline();
  EntityWorld& world = host.getWorld();

  world.observe<ComponentRemovedEvent>(world.component<InventoryComponent>(), {},
                destroyInventoryOnEntityDestruction);

  pipeline.getGroup<GameGroup>().attachToStage<GameGroup::TickStage>(updateItemHands);
}

RAMPAGE_END
