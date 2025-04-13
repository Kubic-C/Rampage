#pragma once

#include "../utility/math.hpp"

struct CameraComponent {
	CameraComponent()
		: m_zoom(1.0f), m_rot(0) {
	}

	glm::mat4 view(const b2Transform& transform, const Vec2& screenDim, float z = 1.0f) const {
		Vec2 cameraPos = Vec2(transform.p);

		glm::mat4 view = glm::identity<glm::mat4>();
		view = glm::lookAt(glm::vec3(cameraPos, z), glm::vec3(cameraPos, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)); \
		view = glm::ortho<float>(-1.0f, 1.0f, -1.0f, 1.0f, 1000.0f, -1000.0f) * view;
		view = glm::rotate(view, -Rot(transform.q).radians(), glm::vec3(0.0f, 0.0f, 1.0f));
		view = glm::scale(view, glm::vec3(m_zoom));
		view = glm::translate(view, glm::vec3(-cameraPos, 0.0f));

		return view;
	}

	void safeZoom(float amount) {
		if (m_zoom + amount < 0.01f)
			return;
		m_zoom += amount;
	}

	template<typename Stream>
	bool Serialize(Stream& stream) {
		return true;
	}

	float m_zoom;
	float m_rot;
};