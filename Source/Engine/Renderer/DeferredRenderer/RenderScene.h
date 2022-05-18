#pragma once
#include "SceneStaticUniform.h"
#include "../../Core/TickData.h"
#include "../../Scene/Component/StaticMeshComponent.h"

namespace flower
{
	class DefferedRenderer;
	class SceneManager;
	class AtmosphereSky;
	struct Mesh;

	// Rendering scene infos misc.
	class RenderScene
	{
	private:
		DefferedRenderer* m_renderer;
		SceneManager* m_sceneManager;

		GPUFrameData m_frameData;
		GPUViewData m_viewData;

		bool m_bAtmosphereActive = false;

		std::vector<RenderMeshProxy> m_cacheStaticMeshProxy;

		std::vector<GPUObjectData>   m_cacheMeshObjectSSBOData {};
		std::vector<GPUMaterialData> m_cacheMeshMaterialSSBOData {};
	public:
		void init();
		void tick(const TickData& tickData);

		const GPUFrameData& getFrameData() const;
		const GPUViewData&  getViewData()  const;

		auto& getMeshObjectSSBOData() { return m_cacheMeshObjectSSBOData; }
		auto& getMeshMaterialSSBOData() { return m_cacheMeshMaterialSSBOData; }

		const std::vector<RenderMeshProxy>& getRenderMeshProxys() const { return m_cacheStaticMeshProxy;  }
		std::shared_ptr<AtmosphereSky> getAtmosphereSky();

		bool ShouldRenderAtmosphere();
	private:
		uint64_t m_frameCount = 0;

		void updateFrameData();
		void updateViewData();

		void meshCollect();
	};
}