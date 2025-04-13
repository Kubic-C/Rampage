#pragma once

#include "../utility/base.hpp"

class BaseRender {
public:
	BaseRender(EntityWorld& world)
		: m_world(world) {}

	virtual void preMesh() {}
	virtual void onMesh() {}
	virtual void preRender() {};
	virtual void onRender(const glm::mat4& vp) = 0;
	virtual void postRender() {};

	Status getStatus() {
		return m_status;
	}

protected:
	Status m_status;
	EntityWorld& m_world;
};