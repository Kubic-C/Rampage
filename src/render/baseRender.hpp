#pragma once

#include "../ecs/ecs.hpp"

struct IsRender {};

class BaseRenderModule : public Module {
public:
	BaseRenderModule(EntityWorld& world, size_t priority)
		: m_world(world), m_priority(priority) {}

	virtual void preMesh() {}
	virtual void onMesh() {}
	virtual void preRender() {};
	virtual void onRender(const glm::mat4& vp) = 0;
	virtual void postRender() {};

	size_t getPriority() const {
		return m_priority;
	}

	void setPriority(size_t priority) {
		m_priority = priority;
	}

	Status getStatus() {
		return m_status;
	}

protected:
	Status m_status = Status::Ok;
	EntityWorld& m_world;
	size_t m_priority;
};