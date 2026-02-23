#pragma once

#include "ihost.hpp"

RAMPAGE_START

/**
 * Defines a pipeline where a series of functions will be called when .run() is called.
 * Stages may be added or removed.
 */
class Pipeline {
  typedef int (*StageTask)(IHost& host, float dt);
  typedef int (*WorldStageTask)(IWorldPtr world, float dt);

  struct Stage {
    size_t name = 0;
    std::vector<StageTask> tasks;
    std::vector<WorldStageTask> worldTasks;
  };

public:
  class Group {
    friend class Pipeline;

  public:
    template <typename Name>
    Group& createStage();

    template <typename Predecessor, typename Name>
    Group& createStageAfter();

    template <typename Successor, typename Name>
    Group& createStageBefore();

    template <typename Name>
    Group& attachToStage(StageTask task);

    template <typename Name>
    Group& attachToStage(WorldStageTask task);

  protected:
    template <typename Name>
    std::list<Stage>::iterator searchStage() {
      return std::ranges::find_if(m_stages,
                                  [](const Stage& stage) { return stage.name == typeid(Name).hash_code(); });
    }

    size_t m_name = 0;
    std::list<Stage> m_stages;
    float m_invRate = -1.0f;
    float m_tickAccum = 0.0f;
  };

  template <typename GroupName>
  Group& createGroup(float rate);

  template <typename GroupName>
  Group& getGroup();

  void run(IHost& host);

private:
  template <typename GroupName>
  std::list<Group>::iterator searchGroup() {
    return std::ranges::find_if(
        m_groups, [](const Group& group) { return group.m_name == typeid(GroupName).hash_code(); });
  }

  std::list<Group> m_groups;
  float m_invDefaultRate = 60;
  float m_lastTime = 0.0f;
};

template <typename GroupName>
Pipeline::Group& Pipeline::createGroup(float rate) {
  m_groups.emplace_back();
  m_groups.back().m_name = typeid(GroupName).hash_code();
  m_groups.back().m_invRate = 1.0f / rate;

  return m_groups.back();
}

template <typename GroupName>
Pipeline::Group& Pipeline::getGroup() {
  const auto it = searchGroup<GroupName>();
  if (it == m_groups.end())
    throw std::runtime_error("Group not found: " + std::string(typeid(GroupName).name()));

  return *it;
}

template <typename Name>
Pipeline::Group& Pipeline::Group::createStage() {
  m_stages.emplace_back();
  m_stages.back().name = typeid(Name).hash_code();

  return *this;
}

template <typename Predecessor, typename Name>
Pipeline::Group& Pipeline::Group::createStageAfter() {
  const auto it = m_stages.insert(++searchStage<Predecessor>(), Stage());
  it->name = typeid(Name).hash_code();

  return *this;
}

template <typename Successor, typename Name>
Pipeline::Group& Pipeline::Group::createStageBefore() {
  const auto it = m_stages.insert(searchStage<Successor>(), Stage());
  it->name = typeid(Name).hash_code();

  return *this;
}

template <typename Name>
Pipeline::Group& Pipeline::Group::attachToStage(StageTask task) {
  searchStage<Name>()->tasks.push_back(task);

  return *this;
}

template <typename Name>
Pipeline::Group& Pipeline::Group::attachToStage(WorldStageTask task) {
  searchStage<Name>()->worldTasks.push_back(task);

  return *this;
}

RAMPAGE_END
