#include "DirectionalLightComponent.h"
#include "../SceneNode.h"
#include "../SceneGraph.h"

namespace flower 
{
	glm::vec4 DirectionalLight::getDirection() const
	{
		if (auto node = m_node.lock())
		{
			const auto worldMatrix = node->getTransform()->getWorldMatrix();
			glm::vec4 forward = glm::vec4(m_forward.x, m_forward.y, m_forward.z, 0.0f);
			return worldMatrix * forward;
		}

		return glm::vec4(0.0f);
	}

	glm::vec3 DirectionalLight::getColor() const
	{
		return m_color;
	}

	bool DirectionalLight::setForward(const glm::vec3& inForward)
	{
		if (m_forward != inForward)
		{
			m_forward = inForward;
			m_node.lock()->getSceneReference().lock()->setDirty(true);

			return true;
		}

		return false;
	}

	bool DirectionalLight::setColor(const glm::vec3& inColor)
	{
		if (m_color != inColor)
		{
			m_color = inColor;
			m_node.lock()->getSceneReference().lock()->setDirty(true);

			return true;
		}

		return false;
	}

	glm::vec3 DirectionalLight::getForward() const
	{
		return m_forward;
	}
}