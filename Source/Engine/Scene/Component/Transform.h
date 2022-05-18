#pragma once
#include "../Component.h"
#include "../../Core/Core.h"

namespace flower
{
	class SceneNode;

	class Transform : public Component
	{
		friend class Scene;

	public:
		Transform() = default;
		COMPONENT_NAME_OVERRIDE(Transform);

		virtual ~Transform() = default;

	private:
		// need update?
		bool m_bUpdateFlag = true;

		glm::vec3 m_translation = { .0f, .0f, .0f };
		glm::quat m_rotation    = { 1.f, .0f, .0f, .0f };
		glm::vec3 m_scale       = { 1.f, 1.f, 1.f };

		// world space matrix.
		glm::mat4 m_worldMatrix = glm::mat4(1.0);
		glm::mat4 m_prevWorldMatrix = glm::mat4(1.0);

	private:
		void updateWorldTransform();

	public:
		// getter
		const glm::vec3& getTranslation() const;
		const glm::quat& getRotation() const;
		const glm::vec3& getScale() const;

		// get local matrix, no relate to parent. 
		glm::mat4 getMatrix() const;

		// mark world matrix dirty, also change child's dirty state.
		void invalidateWorldMatrix();

		// setter.
		void setTranslation(const glm::vec3& translation);
		void setRotation(const glm::quat& rotation);
		void setScale(const glm::vec3& scale);
		void setMatrix(const glm::mat4& matrix);

		// get final world matrix. relate to parent.
		glm::mat4 getWorldMatrix();

		glm::mat4 getPrevWorldMatrix();
	};
}