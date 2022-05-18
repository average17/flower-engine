#pragma once
#include "../RenderTexturePool.h"

namespace flower 
{
	// 128 * 128 blue noise.
	struct GlobalBlueNoise
	{
		// 16 Spp .////////////////////////////////////
		
		// The Sobol sequence buffer.
		VulkanBuffer* sobolBuffer16Spp;

		// The ranking tile buffer for sampling.
		VulkanBuffer* rankingTileBuffer16Spp;

		// The scrambling tile buffer for sampling.
		VulkanBuffer* scramblingTileBuffer16Spp;

		// 1 Spp .////////////////////////////////////

		// The Sobol sequence buffer.
		VulkanBuffer* sobolBuffer1Spp;

		// The ranking tile buffer for sampling.
		VulkanBuffer* rankingTileBuffer1Spp;

		// The scrambling tile buffer for sampling.
		VulkanBuffer* scramblingTileBuffer1Spp;

		// 4 Spp .////////////////////////////////////

		// The Sobol sequence buffer.
		VulkanBuffer* sobolBuffer4Spp;

		// The ranking tile buffer for sampling.
		VulkanBuffer* rankingTileBuffer4Spp;

		// The scrambling tile buffer for sampling.
		VulkanBuffer* scramblingTileBuffer4Spp;


		///..///////////////
		VulkanDescriptorSetReference set = {};
		VulkanDescriptorLayoutReference setLayouts = {};

		~GlobalBlueNoise()
		{
			delete sobolBuffer16Spp;
			delete rankingTileBuffer16Spp;
			delete scramblingTileBuffer16Spp;

			delete sobolBuffer1Spp;
			delete rankingTileBuffer1Spp;
			delete scramblingTileBuffer1Spp;

			delete sobolBuffer4Spp;
			delete rankingTileBuffer4Spp;
			delete scramblingTileBuffer4Spp;
		}
	};

	class EpicAtmosphere
	{
	private:
		bool m_bCloudNoiseInit = false;

	public:
		static const uint32_t TRANSMITTANCE_TEXTURE_WIDTH = 256;
		static const uint32_t TRANSMITTANCE_TEXTURE_HEIGHT = 64;

		static const uint32_t MULTISCATTER_TEXTURE_WIDTH = 32;
		static const uint32_t MULTISCATTER_TEXTURE_HEIGHT = 32;

		static const uint32_t SKYVIEW_TEXTURE_WIDTH = 256;
		static const uint32_t SKYVIEW_TEXTURE_HEIGHT = 256;


		VulkanImage* transmittanceLut = nullptr;
		static VkFormat getTransmittanceLutFormat() { return VK_FORMAT_R16G16B16A16_SFLOAT; }

		VulkanImage* multiScatterLut = nullptr;
		static VkFormat getMultiScatterLutFormat() { return VK_FORMAT_R16G16B16A16_SFLOAT; }

		VulkanImage* skyViewLut = nullptr;
		static VkFormat getSkyViewLutFormat() { return VK_FORMAT_R16G16B16A16_SFLOAT; }

		~EpicAtmosphere()
		{
			delete transmittanceLut;
			delete multiScatterLut;
			delete skyViewLut;
		}
	};

	// Global static textures, viewport size un-relative.
	// for precompute lut.
	class StaticTextures
	{
	private:
		VulkanImage* m_brdfLut = nullptr;
		VulkanImage* m_blueNoise1Spp = nullptr;
		RenderTextureCube* m_fallbackReflectionCapture = nullptr;
		GlobalBlueNoise* m_globalBlueNoise = nullptr;
		EpicAtmosphere* m_epicAtmosphere = nullptr;

	public:
		void release();

		static const uint32_t BRDF_LUT_DIM_XY = 512u;
		VulkanImage* getBRDFLut();

		// global 1spp blue noise.
		static const uint32_t BLUE_NOISE_DIM_XY = 128u;
		VulkanImage* getBlueNoise1Spp();

		static const uint32_t SPECULAR_CAPTURE_DIM_XY = 512u;
		RenderTextureCube* getFallbackReflectionCapture();

		inline static VkFormat getBRDFLutFormat() { return VK_FORMAT_R16G16_SFLOAT; }
		inline static VkFormat getSpecularCaptureFormat() { return VK_FORMAT_R32G32B32A32_SFLOAT; }
		inline static VkFormat getBlueNoiseTextureFormat() { return VK_FORMAT_R8G8_UNORM; }

		// global blue noise common resource.
		GlobalBlueNoise* getGlobalBlueNoise();
		EpicAtmosphere* getEpicAtmosphere();
		
	};

	// Global scene textures. viewport size relative.
	// for gbuffer, for depth stencil, for history.
	class SceneTextures
	{
	private:
		VulkanImage* m_gbufferA = nullptr;
		VulkanImage* m_gbufferB = nullptr;
		VulkanImage* m_gbufferC = nullptr;
		VulkanImage* m_gbufferS = nullptr;

		VulkanImage* m_velocity = nullptr;
		DepthStencilImage* m_depthStencil = nullptr;

		RenderTexture* m_hiz = nullptr;

		VulkanImage* m_hdrSceneColor = nullptr;
		VulkanImage* m_ldrSceneColor = nullptr;
		
		VulkanImage* m_history = nullptr;
		VulkanImage* m_taaImage = nullptr;

		VulkanImage* m_ssrIntersect = nullptr;
		VulkanImage* m_ssrHitMask = nullptr;
		VulkanImage* m_ssrReflection = nullptr;
		VulkanImage* m_ssrTemporalFilter = nullptr;

		VulkanImage* m_ssaoMask = nullptr;

		uint32_t m_renderWidth;
		uint32_t m_renderHeight;

		DepthOnlyImage* m_shadowDepth = nullptr;

		VulkanImage* m_ssrReflectionPrev = nullptr;

		

	

	public:
		VulkanImage* getGbufferA();
		VulkanImage* getGbufferB();
		VulkanImage* getGbufferC();
		VulkanImage* getGbufferS();

		VulkanImage* getVelocity();
		DepthOnlyImage* getShadowDepth();

		RenderTexture* getHiz();

		VulkanImage* getSSRIntersect();
		VulkanImage* getSSRHit();

		VulkanImage* getSSAOMask();

		VulkanImage* getSSRReflection();
		
		VulkanImage* getSSRTemporalFilter();
		VulkanImage* getHistory();
		VulkanImage* getTAA();
		
		VulkanImage* getHdrSceneColor();
		VulkanImage* getLdrSceneColor();
		DepthStencilImage* getDepthStencil();

		VulkanImage* getSSRReflectionPrev();

		

		void release();
		void updateSize(uint32_t width, uint32_t height, bool bForceRebuild);

	public:
		inline static VkFormat getHizFormat() { return VK_FORMAT_R32G32_SFLOAT; }

		inline static VkFormat getSSAOMaskFormat() { return VK_FORMAT_R8G8_UNORM; }
		inline static uint32_t SSAODownScaleSize() { return 1; /* i'm using RTX 3090, no motivation to downsample. */ }

		inline static VkFormat getVelocityFormat() { return VK_FORMAT_R32G32_SFLOAT; }
		inline static VkFormat getGbufferAFormat() { return VK_FORMAT_R8G8B8A8_UNORM;}
		inline static VkFormat getGbufferBFormat() { return VK_FORMAT_R16G16B16A16_SFLOAT;}
		inline static VkFormat getGbufferCFormat() { return VK_FORMAT_R16G16B16A16_SFLOAT;}
		inline static VkFormat getGbufferSFormat() { return VK_FORMAT_R8G8B8A8_UNORM; }
		inline static VkFormat getHdrSceneColorFormat() { return VK_FORMAT_R16G16B16A16_SFLOAT; }
		inline static VkFormat getLdrSceneColorFormat() { return VK_FORMAT_R8G8B8A8_UNORM; }
		inline static VkFormat getDepthStencilFormat() { return RHI::get()->getVulkanDevice()->getDepthStencilFormat(); }
		inline static VkFormat getShadowDepthFormat() { return RHI::get()->getVulkanDevice()->getDepthOnlyFormat(); }


		inline static VkFormat getSSRIntersectFormat() { return VK_FORMAT_R16G16B16A16_SFLOAT; }
		inline static VkFormat getSSRHitMaskFormat() { return VK_FORMAT_R8_UNORM; }

		inline static VkFormat getSSRReflectionFormat() { return SceneTextures::getHdrSceneColorFormat(); }

		inline static VkFormat getTAAFormat() { return SceneTextures::getHdrSceneColorFormat(); }
		inline static VkFormat getHistoryFormat() { return SceneTextures::getHdrSceneColorFormat(); }

		inline static VkFormat getDepthPrevFormat() { return VK_FORMAT_R32_SFLOAT; }
	};
}