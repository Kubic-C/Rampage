#include "module.hpp"

RAMPAGE_START

struct EventData {
  struct KeyData {
    bool pressed = false;
    bool hold = false;
  };

  std::vector<SDL_Event> polledEvents;
  bool signalResized = false;
  Vec2 windowSize;

  std::vector<SDL_Scancode> pressedKeys;
  std::array<KeyData, SDL_SCANCODE_COUNT> keys;
};

int EventModule::onLoad() {
  EntityWorld& world = m_host->getWorld();

  world.addContext<EventData>();

  return 0;
}

int EventModule::onUnload() {
  return 0;
}

int EventModule::onUpdate() {
  auto& eventData = m_host->getWorld().getContext<EventData>();

  eventData.signalResized = false;

  for (SDL_Scancode code : eventData.pressedKeys) {
    eventData.keys[code].pressed = false;
  }
  eventData.pressedKeys.clear();

  eventData.polledEvents.clear();
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    eventData.polledEvents.push_back(event);

    switch ((Event)event.type) {
    case Event::Quit:
      m_host->exit();
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
      break;;
    }
  }

  return 0;
}

bool EventModule::isKeyPressed(Key key) const {
  auto& eventData = m_host->getWorld().getContext<EventData>();

  return eventData.keys[(u32)key].pressed;
}

bool EventModule::isKeyHeld(Key key) const {
  auto& eventData = m_host->getWorld().getContext<EventData>();

  return eventData.keys[(u32)key].hold;
}

bool EventModule::hasWindowResized() const {
  auto& eventData = m_host->getWorld().getContext<EventData>();

  return eventData.signalResized;
}

glm::vec2 EventModule::getWindowSize() const {
  auto& eventData = m_host->getWorld().getContext<EventData>();

  return eventData.windowSize;
}

Vec2 EventModule::getMouseCoords() const {
  float x, y;

  SDL_GetMouseState(&x, &y);

  return Vec2(x, y);
}

RAMPAGE_END