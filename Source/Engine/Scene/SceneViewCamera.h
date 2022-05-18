#pragma once
#include "../Core/TickData.h"
#include "../Core/Core.h"

namespace flower
{
	namespace SceneViewCameraUtils
	{
		const float yawDefault         = -90.0f;
		const float pitchDefault       = 0.0f;
		const float speedDefault       = 10.0f;
		const float maxMoveSpeed       = 20.0f;
		const float minMoveSpeed       = 1.0f;
		const float sensitivityDefault = 0.1f;
		const float zoomDefault        = 60.0f;
		const float zNearDefault       = 0.1f;
		const float zFarDefault        = 20000.0f;

		enum class EMoveType
		{
			Forward,
			Backward,
			Left,
			Right,
		};
	}

	struct Frustum
	{
		std::array<glm::vec4, 6> planes;
	};

	class OffscreenRenderer;
	class SceneViewCamera
	{
	private:
		// world space position.
		glm::vec3 m_position;

		// front direction.
		glm::vec3 m_front;

		// up direction.
		glm::vec3 m_up;

		// worldspace up.
		glm::vec3 m_worldUp;

		// right direction;
		glm::vec3 m_right;

		// yaw and pitch. degree.
		float m_yaw;
		float m_pitch;

		// z near and z far.
		float m_zNear;
		float m_zFar;

		// mouse speed.
		float m_moveSpeed;
		float m_mouseSensitivity;

		// scroll zoom, use for fovy. degree.
		float m_zoom;
		float m_fovY;
		float m_aspect;

		// first time 
		bool  m_bFirstMouse = true;

		float m_lastX;
		float m_lastY;

		uint32_t m_viewWidth;
		uint32_t m_viewHeight;

		glm::mat4 m_viewMatrix{ 1.0f };
		glm::mat4 m_projectMatrix{ 1.0f };

	private: // control details.
		bool m_bActiveViewport = false;
		bool m_bActiveViewportLastframe = false;
		bool m_bHideMouseCursor = false;

		OffscreenRenderer* m_offscreenRenderer;
	public:
		virtual ~SceneViewCamera() = default;

		SceneViewCamera(
			glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
			float yaw = SceneViewCameraUtils::yawDefault,
			float pitch = SceneViewCameraUtils::pitchDefault
		);

		// return camera worldspcae position.
		glm::vec3 getPosition();

		// return camera view matrix.
		glm::mat4 getViewMatrix() const;
		glm::mat4 getProjectMatrix() const;

		float getAspect() const;
		float getFovY() const;
		float getZNear() const;
		float getZFar() const;

		// return worldspace frustum plane.
		Frustum getWorldFrustum() const; 

		void onInit(OffscreenRenderer* renderer);
		void onTick(const TickData& data);

	private:
		void updateCameraVectors();
		void updateMatrixMisc();

		void processKeyboard(SceneViewCameraUtils::EMoveType direction, float deltaTime);
		void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);
		void processMouseScroll(float yoffset);
	};
}