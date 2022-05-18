#pragma once
#include "../Renderer.h"
#include "RenderScene.h"
#include "SceneTextures.h"

namespace flower
{
	class GBufferPass;
	class LightingPass;
	class TonemapperPass;
	class CullingPass;
	class SDSMPass;
	class TAAPass;
	class SpecularCaptureFilterPass;
	class Bloom;
	class HizPass;
	class SSSR;
	class TextureManager;
	class BlueNoisePass;
	class SSAO;
	class EpicAtmospherePass;

	class DefferedRenderer : public OffscreenRenderer
	{
	protected:
		virtual bool init() override;
		virtual void tick(const TickData&) override;
		virtual void release() override;
		
	public:
		DefferedRenderer(ModuleManager* in, std::string name = "DefferedRenderer");
		virtual ~DefferedRenderer() { }

		auto* getFrameDataRing() { return &m_frameDataRing; }
		auto* getViewDataRing() { return &m_viewDataRing; }
		auto* getObjectDataRing() { return &m_objectDataRing; }
		auto* getMaterialDataRing() { return &m_materialDataRing; }

		auto* getDrawIndirectBuffer() { return &m_drawIndirectSSBOGbuffer; }

		VulkanImage* getOutputImage();
		SceneTextures* getSceneTextures() { return &m_sceneTextures; }
		auto* getStaticTextures() { return &m_staticTextures; }
		auto* getRenderSizeChangeCallback() { return &m_renderSizeChangeCallback; }
		RenderScene* getRenderScene() { return &m_renderScene; }
		bool updateRenderSize(uint32_t width, uint32_t height);
		uint64_t getRenderTimes() const { return m_renderTimes; }

	private:
		RenderRingData<GPUFrameData, MAX_FRAMES_IN_FLIGHT> m_frameDataRing;
		RenderRingData<GPUViewData,  MAX_FRAMES_IN_FLIGHT> m_viewDataRing;

		SceneUploadSSBO<GPUObjectData, MAX_FRAMES_IN_FLIGHT>  m_objectDataRing;
		SceneUploadSSBO<GPUMaterialData,MAX_FRAMES_IN_FLIGHT> m_materialDataRing;

		// gbuffer pass culling result.
		DrawIndirectBuffer<
			GPUDrawCallData, 
			MAX_SSBO_OBJECTS, 
			GPUOutIndirectDrawCount, 
			1> 
		m_drawIndirectSSBOGbuffer;

		RenderScene m_renderScene;

		StaticTextures m_staticTextures = {};
		SceneTextures m_sceneTextures = {};

		DelegatesSingleThread<DefferedRenderer,uint32_t,uint32_t> m_renderSizeChangeCallback;

		BlueNoisePass* m_blueNoise = nullptr;
		EpicAtmospherePass* m_epicAtmospherePass = nullptr;

		CullingPass* m_cullingPass = nullptr;
		GBufferPass* m_gbufferPass = nullptr;
		HizPass* m_hizPass = nullptr;

		SDSMPass* m_sdsmPass = nullptr;

		LightingPass* m_lightingPass = nullptr;
		SSSR* m_sssrPass = nullptr;
		SSAO* m_ssaoPass = nullptr;
		TAAPass* m_taaPass = nullptr;

		Bloom* m_bloomPass = nullptr;
		TonemapperPass* m_tonemapperPass = nullptr;

		SpecularCaptureFilterPass* m_specularCapturePass = nullptr;

		TextureManager* m_textureManager = nullptr;
		uint64_t m_renderTimes = 0;

	private:
		void onRenderSizeChange(bool bForceRebuild);
	};
}