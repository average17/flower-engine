#include "RenderScene.h"
#include "DefferedRenderer.h"
#include "../../Scene/SceneManager.h"
#include "../../Engine.h"
#include "../../Core/Timer.h"
#include "../../Scene/SceneViewCamera.h"
#include "../../Scene/Component/DirectionalLightComponent.h"
#include "Pass/TAAPass.h"
#include "../../Scene/Component/AtmosphereSky.h"

namespace flower
{
	void RenderScene::init()
	{
		m_renderer = GEngine.getRuntimeModule<DefferedRenderer>();
		m_sceneManager = GEngine.getRuntimeModule<SceneManager>();

		CHECK(m_sceneManager != nullptr && "you should register scene manager before renderer.");
		CHECK(m_renderer != nullptr && "you should register renderer.");
	}

	void RenderScene::tick(const TickData& tickData)
	{
		updateFrameData();
		updateViewData();
		meshCollect();
	}

	const GPUFrameData& RenderScene::getFrameData() const
	{ 
		return m_frameData; 
	}

	const GPUViewData& RenderScene::getViewData() const
	{ 
		return m_viewData; 
	}

	std::shared_ptr<AtmosphereSky> RenderScene::getAtmosphereSky()
	{
		if(auto atmosphereSky = m_sceneManager->getAtmosphereSky())
		{
			return atmosphereSky;
		}

		return nullptr;
	}

	bool RenderScene::ShouldRenderAtmosphere()
	{
		return getAtmosphereSky() != nullptr;
	}

	// FIXME: when light dir set to (0,-1,0), eular angle to quart will false.
	constexpr auto DEFAULT_LIGHT_DIR = glm::vec3(0.1f, -0.9f, .0f);
	void RenderScene::updateFrameData()
	{
		// we use accumulate global timer as game time.
		float gametime = GTimer.gameTime();

		m_frameData.appTime = glm::vec4(
			GTimer.globalPassTime(),
			gametime,
			glm::sin(gametime),
			glm::cos(gametime)
		);

		m_frameData.directionalLightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		m_frameData.directionalLightDir = glm::vec4(glm::normalize(DEFAULT_LIGHT_DIR), 0.0f);
		if(auto directionalLight = m_sceneManager->getDirectionalLightComponent())
		{
			m_frameData.directionalLightColor = glm::vec4(directionalLight->getColor(), 1.0f);
			m_frameData.directionalLightDir = directionalLight->getDirection();
		}

		// prepare frame index.
		m_frameData.frameIndex = glm::uvec4(m_frameCount, m_frameCount % 8, m_frameCount % 16, m_frameCount % 32);

		// store prev frame jitter data.
		m_frameData.jitterData.z = m_frameData.jitterData.x;
		m_frameData.jitterData.w = m_frameData.jitterData.y;
		m_frameData.switch0.x = TAAOpen();
		if(TAAOpen())
		{
			const float scale = 1.0f;

			// 8 samples are more stable than 16 samples.
			// but 16 samples is very clear.
			const uint8_t samples = 16;
			const uint64_t index = m_frameCount % samples;
			glm::vec2 taaJitter = Halton2D(index, 2, 3);

			float sigma = 0.47f * scale;
			float outWindow = 0.5f;
			float expFactor = outWindow / sigma;
			float inWindow = glm::exp(-0.5f * expFactor * expFactor);

			// Box-Muller transform
			float theta = 2.0f * glm::pi<float>() * taaJitter.y;
			float r = sigma * glm::sqrt(-2.0f * glm::log((1.0f - taaJitter.x) * inWindow + taaJitter.x));

			float sampleX = r * glm::cos(theta) * 2.0f;
			float sampleY = r * glm::sin(theta) * -2.0f;

			m_frameData.jitterData.x = sampleX / (float)m_renderer->getRenderWidth();
			m_frameData.jitterData.y = sampleY / (float)m_renderer->getRenderHeight();
		}
		else
		{
			m_frameData.jitterData = glm::vec4(0.0f);
		}

		m_frameCount ++;

		if (m_frameCount >= 0xFFFFFFFF)
		{
			m_frameCount = 0;
		}
	}

	void RenderScene::updateViewData()
	{
		// prepare prev frame data
		m_viewData.camViewProjLast = m_viewData.camViewProj;

		// current default use scene view camera.
		m_viewData.camWorldPos = glm::vec4(m_sceneManager->getSceneViewCamera()->getPosition(), 1.0f);

		m_viewData.camInfo = glm::vec4(
			m_sceneManager->getSceneViewCamera()->getFovY(),
			m_sceneManager->getSceneViewCamera()->getAspect(),
			m_sceneManager->getSceneViewCamera()->getZNear(),
			m_sceneManager->getSceneViewCamera()->getZFar()
		);


		m_viewData.camProj = m_sceneManager->getSceneViewCamera()->getProjectMatrix();
		m_viewData.camView = m_sceneManager->getSceneViewCamera()->getViewMatrix();
		m_viewData.camViewProj = m_viewData.camProj * m_viewData.camView;

		m_viewData.camProjInverse = glm::inverse(m_viewData.camProj);
		m_viewData.camViewInverse = glm::inverse(m_viewData.camView);
		m_viewData.camViewProjInverse = glm::inverse(m_viewData.camViewProj);

		if (TAAOpen())
		{
			glm::mat4 curJitterMat = glm::mat4(1.0f);
			curJitterMat[3][0] += m_frameData.jitterData.x;
			curJitterMat[3][1] += m_frameData.jitterData.y;

			m_viewData.camViewProjJitter = curJitterMat * m_viewData.camViewProj;
			m_viewData.camProjJitter = curJitterMat * m_viewData.camProj;
		}
		else
		{
			m_viewData.camViewProjJitter = m_viewData.camViewProj;
			m_viewData.camProjJitter = m_viewData.camProj;
		}

		m_viewData.camInvertViewProjectionJitter = glm::inverse(m_viewData.camViewProjJitter);
		m_viewData.camProjJitterInverse = glm::inverse(m_viewData.camProjJitter);

		Frustum currentFrustum = m_sceneManager->getSceneViewCamera()->getWorldFrustum();

		m_viewData.frustumPlanes[0] = currentFrustum.planes[0];
		m_viewData.frustumPlanes[1] = currentFrustum.planes[1];
		m_viewData.frustumPlanes[2] = currentFrustum.planes[2];
		m_viewData.frustumPlanes[3] = currentFrustum.planes[3];
		m_viewData.frustumPlanes[4] = currentFrustum.planes[4];
		m_viewData.frustumPlanes[5] = currentFrustum.planes[5];
	}

	void RenderScene::meshCollect()
	{
		m_cacheMeshObjectSSBOData.clear();
		m_cacheMeshMaterialSSBOData.clear();

		// performance warning.
		// todo: switch to static cache update instead of dynamic update.
		m_sceneManager->collectStaticMesh(m_cacheStaticMeshProxy);

		// then push to ssbo cache data.
		for(auto& proxy : m_cacheStaticMeshProxy)
		{
			for(auto& subMesh : proxy.subMeshes)
			{
				GPUObjectData objData{};
				objData.modelMatrix = proxy.modelMatrix;
				objData.prevModelMatrix = proxy.prevModelMatrix;
				objData.sphereBounds = glm::vec4(subMesh.renderBounds.origin,subMesh.renderBounds.radius);
				objData.extents = glm::vec4(subMesh.renderBounds.extents,1.0f);

				objData.firstInstance = 0;
				objData.vertexOffset = 0;
				objData.indexCount = subMesh.indexCount;
				objData.firstIndex = subMesh.indexStartPosition;

				m_cacheMeshObjectSSBOData.push_back(objData);

				GPUMaterialData matData{};
				matData = subMesh.material.toGPUMaterialData();
				m_cacheMeshMaterialSSBOData.push_back(matData);
			}
		}
	}

	
}