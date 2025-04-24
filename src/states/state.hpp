#pragma once

#include "utility/ecs.hpp"
#include "../contexts.hpp"

class State {
public:
	virtual ~State() = default;

	virtual void onEntry() {}
	virtual void onLeave() {}
	virtual void onTick(u32 tick, float deltaTime) {}
	virtual void onUpdate() {}

protected:
	std::string m_name;
};

class UnknownState : public State {};

class PlayState;

template<typename T>
struct OwnedBy {};

class StateManager {
public:
  template<typename T, typename ... Params>
  void createState(const std::string& name, EntityWorld& world, Params&& ... args) {
    assert(!m_states.contains(name));

    m_states[name] = std::make_shared<T>(world, args...);
  }

  void enableState(const std::string& name) {
    assert(m_states.contains(name));
    m_commandBuffer.push_back(Command{ Command::Type::Enable, name });
  }

  void disableState(const std::string& name) {
    assert(m_states.contains(name));
    m_commandBuffer.push_back(Command{ Command::Type::Disable, name });
  }

  bool isStateEnabled(const std::string& name) {
    return m_enabledStates.contains(name);
  }

  void onTick(u32 tick, float deltaTime) {
    for (Command& command : m_commandBuffer) {
      std::string name(command.name);

      switch (command.type) {
      case Command::Type::Enable: {
        m_enabledStates[name] = m_states[name];
        State& state = *m_states[name];
        state.onEntry();
      } break;
      case Command::Type::Disable: {
        State& state = *m_enabledStates[name];
        state.onLeave();
        m_enabledStates.erase(name);
      } break;
      }
    }
    m_commandBuffer.clear();

    for (auto& [name, statePtr] : m_enabledStates) {
      statePtr->onTick(tick, deltaTime);
    }
  }

  void onUpdate() {
    for (auto& [name, statePtr] : m_enabledStates) {
      statePtr->onUpdate();
    }
  }

private:
  struct Command {
    enum Type {
      Enable,
      Disable
    };

    Type type;
    std::string name;
  };

  std::string m_activeState;
  std::vector<Command> m_commandBuffer;
  Map<std::string, std::shared_ptr<State>> m_states;
  Map<std::string, std::shared_ptr<State>> m_enabledStates;
};