#pragma once

#include "ecs/ecs.hpp"

class Scene {
  public:
  Scene(EntityWorld& world, std::string name, std::string desc) :
      m_world(world), m_name(std::move(name)), m_desc(std::move(desc)) {}

  Scene(Scene&& scene) noexcept :
      m_world(scene.m_world), m_name(std::move(scene.m_name)), m_desc(std::move(scene.m_desc)),
      m_prefabs(std::move(scene.m_prefabs)), m_override(std::move(scene.m_override)) {}

  void addPrefab(EntityId prefab, EntityId override) {
    m_prefabs.push_back(prefab);
    m_override.push_back(override);
  }

  void load(ComponentId sceneTag) {
    m_sceneTag = sceneTag;

    for (int i = 0; i < m_prefabs.size(); i++) {
      Entity clone = m_world.get(m_prefabs[i]).clone();
      clone.add(m_sceneTag);
      if (m_override[i] != NullEntityId)
        m_world.get(m_override[i]).copyInto(clone);
    }
  }

  void unload() {
    m_world.destroyAllEntitiesWith({m_sceneTag});

    m_sceneTag = NullComponentId;
  }

  private:
  EntityWorld& m_world;
  std::string m_name;
  std::string m_desc;

  ComponentId m_sceneTag = NullComponentId;
  std::vector<EntityId> m_prefabs;
  std::vector<EntityId> m_override;
};
