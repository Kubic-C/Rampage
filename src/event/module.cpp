#include "module.hpp"

RAMPAGE_START

struct EventData {
  struct KeyData {
    bool pressed = false;
    bool hold = false;
  };

  EntityId sdlEventEntity;
  bool signalResized = false;
  Vec2 windowSize;

  std::vector<SDL_Scancode> pressedKeys;
  std::array<KeyData, SDL_SCANCODE_COUNT> keys;
};

int handleWindowEvents(IHost& host, float dt) {
  IWorldPtr world = host.getWorld();
  auto& eventData = world->getContext<EventData>();

  eventData.signalResized = false;

  for (SDL_Scancode code : eventData.pressedKeys) {
    eventData.keys[code].pressed = false;
  }
  eventData.pressedKeys.clear();

  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    EntityPtr sdlEventEntity = world->getEntity(eventData.sdlEventEntity);
    *sdlEventEntity.get<SDL_Event>() = event;
    world->emit<SDL_Event>(sdlEventEntity, world->component<SDL_Event>());

    switch (static_cast<Event>(event.type)) {
    case Event::Quit:
      host.exit();
      break;
    case Event::WindowResized:
      eventData.signalResized = true;
      eventData.windowSize.x = event.window.data1;
      eventData.windowSize.y = event.window.data2;
      break;
    case Event::KeyDown:
      eventData.keys[event.key.scancode].pressed = true;
      eventData.keys[event.key.scancode].hold = true;
      eventData.pressedKeys.push_back(event.key.scancode);
      break;
    case Event::KeyUp:
      eventData.keys[event.key.scancode].hold = false;
      break;
    default:
      break;
    }
  }

  return 0;
}

int EventModule::onLoad() {
  IWorldPtr world = m_host->getWorld();

  world->addContext<EventData>();
  auto& eventData = world->getContext<EventData>();

  EntityPtr entity = world->create();
  entity.add<SDL_Event>();
  eventData.sdlEventEntity = entity;

  m_host->getPipeline().getGroup<GameGroup>().attachToStage<GameGroup::PreTickStage>(handleWindowEvents);

  return 0;
}

int EventModule::onUpdate() {
  return 0;
}

bool EventModule::isKeyPressed(Key key) const {
  auto& eventData = m_host->getWorld()->getContext<EventData>();

  return eventData.keys[(u32)key].pressed;
}

bool EventModule::isKeyHeld(Key key) const {
  auto& eventData = m_host->getWorld()->getContext<EventData>();

  return eventData.keys[(u32)key].hold;
}

bool EventModule::hasWindowResized() const {
  auto& eventData = m_host->getWorld()->getContext<EventData>();

  return eventData.signalResized;
}

glm::vec2 EventModule::getWindowSize() const {
  auto& eventData = m_host->getWorld()->getContext<EventData>();

  return eventData.windowSize;
}

Vec2 EventModule::getMouseCoords() const {
  float x, y;

  SDL_GetMouseState(&x, &y);

  return Vec2(x, y);
}

Vec2 EventModule::getMouseWorldPos() const {
  auto renderModule = m_host->getWorld()->getContext<RenderModule>();

  Vec2 mouseCoords = getMouseCoords();
  return renderModule.getWorldCoords(mouseCoords);
}

RAMPAGE_END
