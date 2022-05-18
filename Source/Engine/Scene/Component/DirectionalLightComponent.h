#pragma once
#include "../Component.h"
#include "../../Core/Core.h"

namespace flower
{
	class DirectionalLight : public Component
	{
		friend class Scene;

	public:
		DirectionalLight() = default;
		COMPONENT_NAME_OVERRIDE(DirectionalLight);

		virtual ~DirectionalLight() = default;

	private:
		glm::vec3 m_color = glm::vec3(1.0f,1.0f,1.0f);
		glm::vec3 m_forward = glm::normalize(glm::vec3(-0.1f, -0.9f, 0.0f));

	public:
		glm::vec4 getDirection() const;
		glm::vec3 getColor() const;

		glm::vec3 getForward() const;

		bool setForward(const glm::vec3& in);
		bool setColor(const glm::vec3& in);
	};
}