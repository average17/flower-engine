#include "SceneViewCamera.h"
#include "../Core/WindowData.h"
#include "../Renderer/Renderer.h"

namespace flower
{
	SceneViewCamera::SceneViewCamera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
		: m_position(position) 
		, m_worldUp(up) // assign default world up.
		, m_yaw(yaw)
		, m_pitch(pitch)
		, m_front(glm::vec3(0.0f, 0.0f, 1.0f))
		, m_mouseSensitivity(SceneViewCameraUtils::sensitivityDefault)
		, m_moveSpeed(SceneViewCameraUtils::speedDefault)
		, m_zoom(SceneViewCameraUtils::zoomDefault)
		, m_viewWidth(MIN_RENDER_SIZE)
		, m_viewHeight(MIN_RENDER_SIZE)
		, m_zNear(SceneViewCameraUtils::zNearDefault)
		, m_zFar(SceneViewCameraUtils::zFarDefault)
	{
		updateCameraVectors();
	}

	void SceneViewCamera::updateCameraVectors()
	{
		// get front vector from yaw and pitch angel.
		glm::vec3 front;
		front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
		front.y = sin(glm::radians(m_pitch));
		front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
		m_front = glm::normalize(front);

		// double cross to get camera true up and right vector.
		m_right = glm::normalize(glm::cross(m_front, m_worldUp));
		m_up    = glm::normalize(glm::cross(m_right, m_front));
	}

	glm::vec3 SceneViewCamera::getPosition()
	{
		return m_position;
	}

	void SceneViewCamera::processKeyboard(SceneViewCameraUtils::EMoveType direction, float deltaTime)
	{
		float velocity = m_moveSpeed * deltaTime;

		if (direction == SceneViewCameraUtils::EMoveType::Forward)
			m_position += m_front * velocity;
		if (direction == SceneViewCameraUtils::EMoveType::Backward)
			m_position -= m_front * velocity;
		if (direction == SceneViewCameraUtils::EMoveType::Left)
			m_position -= m_right * velocity;
		if (direction == SceneViewCameraUtils::EMoveType::Right)
			m_position += m_right * velocity;
	}

	void SceneViewCamera::processMouseMovement(float xoffset, float yoffset, bool constrainPitch)
	{
		xoffset *= m_mouseSensitivity;
		yoffset *= m_mouseSensitivity;

		m_yaw += xoffset;
		m_pitch += yoffset;

		if (constrainPitch)
		{
			if (m_pitch > 89.0f)
				m_pitch = 89.0f;
			if (m_pitch < -89.0f)
				m_pitch = -89.0f;
		}

		updateCameraVectors();
	}

	void SceneViewCamera::processMouseScroll(float yoffset)
	{
		m_moveSpeed += (float)yoffset;
		m_moveSpeed = glm::clamp(m_moveSpeed, 
			SceneViewCameraUtils::minMoveSpeed, SceneViewCameraUtils::maxMoveSpeed);
	}

	void SceneViewCamera::onTick(const TickData& tickData)
	{
		uint32_t renderWidth = m_offscreenRenderer->getRenderWidth();
		uint32_t renderHeight = m_offscreenRenderer->getRenderHeight();

		auto* input = GLFWWindowData::get();
		float dt = tickData.deltaTime;

		// prepare view size.
		if (m_viewWidth != renderWidth) m_viewWidth = std::max(MIN_RENDER_SIZE,renderWidth);
		if (m_viewHeight != renderHeight) m_viewHeight = std::max(MIN_RENDER_SIZE,renderHeight);

		// handle first input.
		if (m_bFirstMouse)
		{
			m_lastX = input->getMouseX();
			m_lastY = input->getMouseY();

			m_bFirstMouse = false;
		}

		// handle active viewport state.
		m_bActiveViewport = false;
		if (input->isMouseInViewport())
		{
			if(input->isMouseButtonPressed(Mouse::ButtonRight))
			{
				m_bActiveViewport = true;
			}
		}

		// active viewport. disable cursor.
		if (m_bActiveViewport && !m_bHideMouseCursor)
		{
			m_bHideMouseCursor = true;
			glfwSetInputMode(input->getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}

		// un-active viewport. enable cursor.
		if (!m_bActiveViewport && m_bHideMouseCursor)
		{
			m_bHideMouseCursor = false;
			glfwSetInputMode(input->getWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}

		// first time un-active viewport.
		if (m_bActiveViewportLastframe && !m_bActiveViewport) 
		{
			m_lastX = input->getMouseX();
			m_lastY = input->getMouseY();
		}

		// continue active viewport.
		float xoffset = 0.0f;
		float yoffset = 0.0f;
		if (m_bActiveViewportLastframe && m_bActiveViewport)
		{
			xoffset = input->getMouseX() - m_lastX;
			yoffset = m_lastY - input->getMouseY();
		}

		// update state.
		m_bActiveViewportLastframe = m_bActiveViewport;
		if (m_bActiveViewport)
		{
			m_lastX = input->getMouseX();
			m_lastY = input->getMouseY();

			processMouseMovement(xoffset, yoffset);
			processMouseScroll(input->getScrollOffset().y);

			if (input->isKeyPressed(Key::W))
				processKeyboard(SceneViewCameraUtils::EMoveType::Forward, dt);
			if (input->isKeyPressed(Key::S))
				processKeyboard(SceneViewCameraUtils::EMoveType::Backward, dt);
			if (input->isKeyPressed(Key::A))
				processKeyboard(SceneViewCameraUtils::EMoveType::Left, dt);
			if (input->isKeyPressed(Key::D))
				processKeyboard(SceneViewCameraUtils::EMoveType::Right, dt);
		}

		updateMatrixMisc();
	}

	Frustum SceneViewCamera::getWorldFrustum() const
	{
		const glm::vec3 forwardVector = glm::normalize(m_front);
		const glm::vec3 camWorldPos = m_position;

		const glm::vec3 nearC = camWorldPos + forwardVector * m_zNear;
		const glm::vec3 farC = camWorldPos + forwardVector * m_zFar;

		const float tanFovyHalf = glm::tan(getFovY() * 0.5f);
		const float aspect = getAspect();



		const float yNearHalf = m_zNear * tanFovyHalf;
		const float yFarHalf = m_zFar * tanFovyHalf;

		const glm::vec3 yNearHalfV = yNearHalf * m_up;
		const glm::vec3 xNearHalfV = yNearHalf * aspect * m_right;

		
			
		const glm::vec3 yFarHalfV = yFarHalf * m_up;
		const glm::vec3 xFarHalfV = yFarHalf * aspect * m_right;

		const glm::vec3 NRT = nearC + xNearHalfV + yNearHalfV;
		const glm::vec3 NRD = nearC + xNearHalfV - yNearHalfV;
		const glm::vec3 NLT = nearC - xNearHalfV + yNearHalfV;
		const glm::vec3 NLD = nearC - xNearHalfV - yNearHalfV;

		const glm::vec3 FRT = farC + xFarHalfV + yFarHalfV;
		const glm::vec3 FRD = farC + xFarHalfV - yFarHalfV;
		const glm::vec3 FLT = farC - xFarHalfV + yFarHalfV;
		const glm::vec3 FLD = farC - xFarHalfV - yFarHalfV;

		Frustum ret{};
		
		// p1 X p2, center is pC.
		auto getNormal = [](const glm::vec3& pC, const glm::vec3& p1, const glm::vec3& p2)
		{
			const glm::vec3 dir0 = p1 - pC;
			const glm::vec3 dir1 = p2 - pC;
			const glm::vec3 crossDir = glm::cross(dir0, dir1);
			return glm::normalize(crossDir);
		};

		// left 
		const glm::vec3 leftN = getNormal(FLD, FLT, NLD);
		ret.planes[0] = glm::vec4(leftN, -glm::dot(leftN, FLD));

		// down
		const glm::vec3 downN = getNormal(FRD, FLD, NRD);
		ret.planes[1] = glm::vec4(downN, -glm::dot(downN, FRD));

		// right
		const glm::vec3 rightN = getNormal(FRT, FRD, NRT);
		ret.planes[2] = glm::vec4(rightN, -glm::dot(rightN, FRT));

		// top
		const glm::vec3 topN = getNormal(FLT, FRT, NLT);
		ret.planes[3] = glm::vec4(topN, -glm::dot(topN, FLT));

		// front
		const glm::vec3 frontN = getNormal(NRT, NRD, NLT);
		ret.planes[4] = glm::vec4(frontN, -glm::dot(frontN, NRT));

		// back
		const glm::vec3 backN = getNormal(FRT, FLT, FRD);
		ret.planes[5] = glm::vec4(backN, -glm::dot(backN, FRT));

		return ret;
	}

	void SceneViewCamera::onInit(OffscreenRenderer* renderer)
	{
		m_offscreenRenderer = renderer;
	}

	glm::mat4 SceneViewCamera::getViewMatrix() const
	{
		return m_viewMatrix;
	}

	void SceneViewCamera::updateMatrixMisc()
	{
		m_fovY = glm::radians(m_zoom);
		m_aspect = (float)m_viewWidth / (float)m_viewHeight;

		// update view matrix.
		m_viewMatrix = glm::lookAt(m_position, m_position + m_front, m_up);

		// reverse z.
		m_projectMatrix = glm::perspective(m_fovY, m_aspect, m_zFar, m_zNear);
	}


	glm::mat4 SceneViewCamera::getProjectMatrix() const
	{
		return m_projectMatrix;
	}

	float SceneViewCamera::getAspect() const
	{
		return m_aspect;
	}

	float SceneViewCamera::getFovY() const
	{
		return m_fovY;
	}

	float SceneViewCamera::getZNear() const
	{
		return m_zNear;
	}

	float SceneViewCamera::getZFar() const
	{
		return m_zFar;
	}
}