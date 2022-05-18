#include "SceneTextures.h"
#include "../../RHI/RHI.h"
#include "Pass/SDSMPass.h"

namespace flower 
{
	VulkanImage* SceneTextures::getGbufferA()
	{
		if(m_gbufferA == nullptr)
		{
			m_gbufferA = RenderTexture::create(
				RHI::get()->getVulkanDevice(),
				m_renderWidth,
				m_renderHeight,
				getGbufferAFormat()
			);
		}

		return m_gbufferA;
	}

	VulkanImage* SceneTextures::getGbufferB()
	{
		if(m_gbufferB == nullptr)
		{
			m_gbufferB = RenderTexture::create(
				RHI::get()->getVulkanDevice(),
				m_renderWidth,
				m_renderHeight,
				getGbufferBFormat()
			);
		}

		return m_gbufferB;
	}

	VulkanImage* SceneTextures::getGbufferC()
	{
		if(m_gbufferC == nullptr)
		{
			m_gbufferC = RenderTexture::create(
				RHI::get()->getVulkanDevice(),
				m_renderWidth,
				m_renderHeight,
				getGbufferCFormat()
			);
		}

		return m_gbufferC;
	}

	VulkanImage* SceneTextures::getGbufferS()
	{
		if(m_gbufferS == nullptr)
		{
			m_gbufferS = RenderTexture::create(
				RHI::get()->getVulkanDevice(),
				m_renderWidth,
				m_renderHeight,
				getGbufferSFormat()
			);
		}

		return m_gbufferS;
	}

	VulkanImage* SceneTextures::getVelocity()
	{
		if (m_velocity == nullptr)
		{
			m_velocity = RenderTexture::create(
				RHI::get()->getVulkanDevice(),
				m_renderWidth,
				m_renderHeight,
				getVelocityFormat()
			);
		}

		return m_velocity;
	}

	VulkanImage* SceneTextures::getHdrSceneColor()
	{
		if(m_hdrSceneColor == nullptr)
		{
			m_hdrSceneColor = RenderTexture::create(
				RHI::get()->getVulkanDevice(),
				m_renderWidth,
				m_renderHeight,
				getHdrSceneColorFormat()
			);
		}

		return m_hdrSceneColor;
	}

	VulkanImage* SceneTextures::getLdrSceneColor()
	{
		if(m_ldrSceneColor == nullptr)
		{
			m_ldrSceneColor = RenderTexture::create(
				RHI::get()->getVulkanDevice(),
				m_renderWidth,
				m_renderHeight,
				getLdrSceneColorFormat()
			);

			// ldr scene color init layout set to shader readonly.
			m_ldrSceneColor->transitionLayoutImmediately(
				RHI::get()->getGraphicsCommandPool(),
				RHI::get()->getGraphicsQueue(),
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			);
		}

		return m_ldrSceneColor;
	}

	DepthStencilImage* SceneTextures::getDepthStencil()
	{
		if(m_depthStencil == nullptr)
		{
			m_depthStencil = DepthStencilImage::create(
				RHI::get()->getVulkanDevice(),
				m_renderWidth,
				m_renderHeight,
				true
			);
		}

		return m_depthStencil;
	}

	DepthOnlyImage* SceneTextures::getShadowDepth()
	{
		if (m_shadowDepth == nullptr)
		{
			const auto pageSize = getShadowPerPageDimXY();
			const auto pageCount = getShadowCascadeCount();

			m_shadowDepth = DepthOnlyImage::create(
				RHI::get()->getVulkanDevice(),
				pageSize * pageCount,
				pageSize
			);

			m_shadowDepth->transitionLayoutImmediately(RHI::get()->getGraphicsCommandPool(), RHI::get()->getGraphicsQueue(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);
		}

		return m_shadowDepth;
	}

	RenderTexture* SceneTextures::getHiz()
	{
		if (m_hiz == nullptr)
		{
			m_hiz = RenderTexture::create(
				RHI::get()->getVulkanDevice(),
				m_renderWidth,
				m_renderHeight,
				-1,
				true,
				getHizFormat(),
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT
			);
		}

		return m_hiz;
	}

	VulkanImage* SceneTextures::getHistory()
	{
		if (m_history == nullptr)
		{
			m_history = RenderTexture::create(
				RHI::get()->getVulkanDevice(),
				m_renderWidth,
				m_renderHeight,
				getHistoryFormat()
			);
		}

		return m_history;
	}

	VulkanImage* SceneTextures::getTAA()
	{
		if (m_taaImage == nullptr)
		{
			m_taaImage = RenderTexture::create(
				RHI::get()->getVulkanDevice(),
				m_renderWidth,
				m_renderHeight,
				getTAAFormat()
			);
		}

		return m_taaImage;
	}
	VulkanImage* SceneTextures::getSSRHit()
	{
		if (m_ssrHitMask == nullptr)
		{
			m_ssrHitMask = RenderTexture::create(
				RHI::get()->getVulkanDevice(),
				m_renderWidth,
				m_renderHeight,
				getSSRHitMaskFormat() 
			);

			m_ssrHitMask->transitionLayoutImmediately(RHI::get()->getGraphicsCommandPool(), RHI::get()->getGraphicsQueue(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		return m_ssrHitMask;
	}

	VulkanImage* SceneTextures::getSSAOMask()
	{
		if (m_ssaoMask == nullptr)
		{
			m_ssaoMask = RenderTexture::create(
				RHI::get()->getVulkanDevice(),
				m_renderWidth / SSAODownScaleSize(),
				m_renderHeight / SSAODownScaleSize(),
				getSSAOMaskFormat()
			);

			m_ssaoMask->transitionLayoutImmediately(RHI::get()->getGraphicsCommandPool(), RHI::get()->getGraphicsQueue(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		return m_ssaoMask;
	}

	VulkanImage* SceneTextures::getSSRReflection()
	{
		if (m_ssrReflection == nullptr)
		{
			m_ssrReflection = RenderTexture::create(
				RHI::get()->getVulkanDevice(),
				m_renderWidth,
				m_renderHeight,
				getSSRReflectionFormat()
			);

			m_ssrReflection->transitionLayoutImmediately(RHI::get()->getGraphicsCommandPool(), RHI::get()->getGraphicsQueue(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		return m_ssrReflection;
	}

	VulkanImage* SceneTextures::getSSRReflectionPrev()
	{
		if (m_ssrReflectionPrev == nullptr)
		{
			m_ssrReflectionPrev = RenderTexture::create(
				RHI::get()->getVulkanDevice(),
				m_renderWidth,
				m_renderHeight,
				getSSRReflectionFormat()
			);

			m_ssrReflectionPrev->transitionLayoutImmediately(RHI::get()->getGraphicsCommandPool(), RHI::get()->getGraphicsQueue(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		return m_ssrReflectionPrev;
	}

	VulkanImage* SceneTextures::getSSRTemporalFilter()
	{
		if (m_ssrTemporalFilter == nullptr)
		{
			m_ssrTemporalFilter = RenderTexture::create(
				RHI::get()->getVulkanDevice(),
				m_renderWidth,
				m_renderHeight,
				getSSRReflectionFormat()
			);

			m_ssrTemporalFilter->transitionLayoutImmediately(RHI::get()->getGraphicsCommandPool(), RHI::get()->getGraphicsQueue(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		return m_ssrTemporalFilter;
	}

	VulkanImage* SceneTextures::getSSRIntersect()
	{
		if (m_ssrIntersect == nullptr)
		{
			m_ssrIntersect = RenderTexture::create(
				RHI::get()->getVulkanDevice(),
				m_renderWidth,
				m_renderHeight,
				getSSRIntersectFormat()
			);

			m_ssrIntersect->transitionLayoutImmediately(RHI::get()->getGraphicsCommandPool(), RHI::get()->getGraphicsQueue(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		return m_ssrIntersect;
	}

	void SceneTextures::release()
	{
	#define SAFE_RELEASE(RT) \
		if(RT != nullptr) \
		{\
			delete RT;\
			RT = nullptr;\
		}

		SAFE_RELEASE(m_gbufferA);
		SAFE_RELEASE(m_gbufferB);
		SAFE_RELEASE(m_gbufferC);
		SAFE_RELEASE(m_gbufferS);
		SAFE_RELEASE(m_hdrSceneColor);
		SAFE_RELEASE(m_ldrSceneColor);
		SAFE_RELEASE(m_depthStencil);
		SAFE_RELEASE(m_shadowDepth);
		SAFE_RELEASE(m_velocity);
		SAFE_RELEASE(m_hiz);
		SAFE_RELEASE(m_taaImage);
		SAFE_RELEASE(m_history);

		SAFE_RELEASE(m_ssaoMask);

		SAFE_RELEASE(m_ssrHitMask);
		SAFE_RELEASE(m_ssrIntersect);

		SAFE_RELEASE(m_ssrReflection);
		SAFE_RELEASE(m_ssrReflectionPrev);
		SAFE_RELEASE(m_ssrTemporalFilter);
	#undef SAFE_RELEASE
	}

	void SceneTextures::updateSize(uint32_t width,uint32_t height, bool bForceRebuild)
	{
		if(bForceRebuild || m_renderWidth != width || m_renderHeight != height)
		{
			m_renderWidth = width;
			m_renderHeight = height;

			RHI::get()->waitIdle();
			release();
		}
	}

	void StaticTextures::release()
	{
	#define SAFE_RELEASE(RT) \
		if(RT != nullptr) \
		{\
			delete RT;\
			RT = nullptr;\
		}

		SAFE_RELEASE(m_brdfLut);
		SAFE_RELEASE(m_blueNoise1Spp);
		SAFE_RELEASE(m_fallbackReflectionCapture);
		SAFE_RELEASE(m_globalBlueNoise);
		SAFE_RELEASE(m_epicAtmosphere);

	#undef SAFE_RELEASE
	}

	VulkanImage* StaticTextures::getBRDFLut()
	{
		if (m_brdfLut == nullptr)
		{
			m_brdfLut = RenderTexture::create(
				RHI::get()->getVulkanDevice(),
				BRDF_LUT_DIM_XY,
				BRDF_LUT_DIM_XY,
				getBRDFLutFormat()
			);
		}

		return m_brdfLut;
	}

	VulkanImage* StaticTextures::getBlueNoise1Spp()
	{
		if (m_blueNoise1Spp == nullptr)
		{
			m_blueNoise1Spp = RenderTexture::create(
				RHI::get()->getVulkanDevice(),
				BLUE_NOISE_DIM_XY,
				BLUE_NOISE_DIM_XY,
				getBlueNoiseTextureFormat()
			);

			m_blueNoise1Spp->transitionLayoutImmediately(RHI::get()->getGraphicsCommandPool(), RHI::get()->getGraphicsQueue(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		return m_blueNoise1Spp;
	}

	RenderTextureCube* StaticTextures::getFallbackReflectionCapture()
	{
		if (m_fallbackReflectionCapture == nullptr)
		{
			m_fallbackReflectionCapture = RenderTextureCube::create(
				RHI::get()->getVulkanDevice(),
				SPECULAR_CAPTURE_DIM_XY,
				SPECULAR_CAPTURE_DIM_XY,
				-1,
				getSpecularCaptureFormat(),
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
				true
			);
		}

		return m_fallbackReflectionCapture;
	}

	namespace blueNoise16Spp
	{
		// blue noise sampler 16spp.
		#include <samplerCPP/samplerBlueNoiseErrorDistribution_128x128_OptimizedFor_2d2d2d2d_16spp.cpp>
	}

	namespace blueNoise1Spp
	{
		// blue noise sampler 1spp.
		#include <samplerCPP/samplerBlueNoiseErrorDistribution_128x128_OptimizedFor_2d2d2d2d_1spp.cpp>
	}

	namespace blueNoise4Spp
	{
		// blue noise sampler 4spp.
		#include <samplerCPP/samplerBlueNoiseErrorDistribution_128x128_OptimizedFor_2d2d2d2d_4spp.cpp>
	}

	GlobalBlueNoise* StaticTextures::getGlobalBlueNoise()
	{
		if (m_globalBlueNoise == nullptr)
		{
			m_globalBlueNoise = new GlobalBlueNoise();

			// 16 spp /////////////////////////////////////////////////////////
			m_globalBlueNoise->sobolBuffer16Spp = VulkanBuffer::create(
				RHI::get()->getVulkanDevice(),
				RHI::get()->getGraphicsCommandPool(),
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				VMA_MEMORY_USAGE_CPU_TO_GPU,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				sizeof(blueNoise16Spp::sobol_256spp_256d),
				(void*)blueNoise16Spp::sobol_256spp_256d
			);

			m_globalBlueNoise->rankingTileBuffer16Spp = VulkanBuffer::create(
				RHI::get()->getVulkanDevice(),
				RHI::get()->getGraphicsCommandPool(),
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				VMA_MEMORY_USAGE_CPU_TO_GPU,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				sizeof(blueNoise16Spp::rankingTile),
				(void*)blueNoise16Spp::rankingTile
			);

			m_globalBlueNoise->scramblingTileBuffer16Spp = VulkanBuffer::create(
				RHI::get()->getVulkanDevice(),
				RHI::get()->getGraphicsCommandPool(),
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				VMA_MEMORY_USAGE_CPU_TO_GPU,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				sizeof(blueNoise16Spp::scramblingTile),
				(void*)blueNoise16Spp::scramblingTile
			);

			VkDescriptorBufferInfo sobolInfo16Spp{};
			sobolInfo16Spp.buffer = m_globalBlueNoise->sobolBuffer16Spp->getVkBuffer();
			sobolInfo16Spp.offset = 0;
			sobolInfo16Spp.range = sizeof(blueNoise16Spp::sobol_256spp_256d);

			VkDescriptorBufferInfo rankingTileInfo16Spp{};
			rankingTileInfo16Spp.buffer = m_globalBlueNoise->rankingTileBuffer16Spp->getVkBuffer();
			rankingTileInfo16Spp.offset = 0;
			rankingTileInfo16Spp.range = sizeof(blueNoise16Spp::rankingTile);

			VkDescriptorBufferInfo scramblingTileInfo16Spp{};
			scramblingTileInfo16Spp.buffer = m_globalBlueNoise->scramblingTileBuffer16Spp->getVkBuffer();
			scramblingTileInfo16Spp.offset = 0;
			scramblingTileInfo16Spp.range = sizeof(blueNoise16Spp::scramblingTile);
	
			/// 1 Spp /////////////////////////////////////////////////////////
			m_globalBlueNoise->sobolBuffer1Spp = VulkanBuffer::create(
				RHI::get()->getVulkanDevice(),
				RHI::get()->getGraphicsCommandPool(),
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				VMA_MEMORY_USAGE_CPU_TO_GPU,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				sizeof(blueNoise1Spp::sobol_256spp_256d),
				(void*)blueNoise1Spp::sobol_256spp_256d
			);

			m_globalBlueNoise->rankingTileBuffer1Spp = VulkanBuffer::create(
				RHI::get()->getVulkanDevice(),
				RHI::get()->getGraphicsCommandPool(),
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				VMA_MEMORY_USAGE_CPU_TO_GPU,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				sizeof(blueNoise1Spp::rankingTile),
				(void*)blueNoise1Spp::rankingTile
			);

			m_globalBlueNoise->scramblingTileBuffer1Spp = VulkanBuffer::create(
				RHI::get()->getVulkanDevice(),
				RHI::get()->getGraphicsCommandPool(),
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				VMA_MEMORY_USAGE_CPU_TO_GPU,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				sizeof(blueNoise1Spp::scramblingTile),
				(void*)blueNoise1Spp::scramblingTile
			);

			VkDescriptorBufferInfo sobolInfo1Spp = {};
			sobolInfo1Spp.buffer = m_globalBlueNoise->sobolBuffer1Spp->getVkBuffer();
			sobolInfo1Spp.offset = 0;
			sobolInfo1Spp.range = sizeof(blueNoise1Spp::sobol_256spp_256d);

			VkDescriptorBufferInfo rankingTileInfo1Spp = {};
			rankingTileInfo1Spp.buffer = m_globalBlueNoise->rankingTileBuffer1Spp->getVkBuffer();
			rankingTileInfo1Spp.offset = 0;
			rankingTileInfo1Spp.range = sizeof(blueNoise1Spp::rankingTile);

			VkDescriptorBufferInfo scramblingTileInfo1Spp = {};
			scramblingTileInfo1Spp.buffer = m_globalBlueNoise->scramblingTileBuffer1Spp->getVkBuffer();
			scramblingTileInfo1Spp.offset = 0;
			scramblingTileInfo1Spp.range = sizeof(blueNoise1Spp::scramblingTile);

			/// 4 Spp /////////////////////////////////////////////////////////
			m_globalBlueNoise->sobolBuffer4Spp = VulkanBuffer::create(
				RHI::get()->getVulkanDevice(),
				RHI::get()->getGraphicsCommandPool(),
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				VMA_MEMORY_USAGE_CPU_TO_GPU,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				sizeof(blueNoise4Spp::sobol_256spp_256d),
				(void*)blueNoise4Spp::sobol_256spp_256d
			);

			m_globalBlueNoise->rankingTileBuffer4Spp = VulkanBuffer::create(
				RHI::get()->getVulkanDevice(),
				RHI::get()->getGraphicsCommandPool(),
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				VMA_MEMORY_USAGE_CPU_TO_GPU,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				sizeof(blueNoise4Spp::rankingTile),
				(void*)blueNoise4Spp::rankingTile
			);

			m_globalBlueNoise->scramblingTileBuffer4Spp = VulkanBuffer::create(
				RHI::get()->getVulkanDevice(),
				RHI::get()->getGraphicsCommandPool(),
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				VMA_MEMORY_USAGE_CPU_TO_GPU,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				sizeof(blueNoise4Spp::scramblingTile),
				(void*)blueNoise4Spp::scramblingTile
			);

			VkDescriptorBufferInfo sobolInfo4Spp = {};
			sobolInfo4Spp.buffer = m_globalBlueNoise->sobolBuffer4Spp->getVkBuffer();
			sobolInfo4Spp.offset = 0;
			sobolInfo4Spp.range = sizeof(blueNoise4Spp::sobol_256spp_256d);

			VkDescriptorBufferInfo rankingTileInfo4Spp = {};
			rankingTileInfo4Spp.buffer = m_globalBlueNoise->rankingTileBuffer4Spp->getVkBuffer();
			rankingTileInfo4Spp.offset = 0;
			rankingTileInfo4Spp.range = sizeof(blueNoise4Spp::rankingTile);

			VkDescriptorBufferInfo scramblingTileInfo4Spp = {};
			scramblingTileInfo4Spp.buffer = m_globalBlueNoise->scramblingTileBuffer4Spp->getVkBuffer();
			scramblingTileInfo4Spp.offset = 0;
			scramblingTileInfo4Spp.range = sizeof(blueNoise4Spp::scramblingTile);

			RHI::get()->vkDescriptorFactoryBegin()
				// 16 spp
				.bindBuffer(0, &sobolInfo16Spp, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
				.bindBuffer(1, &rankingTileInfo16Spp, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
				.bindBuffer(2, &scramblingTileInfo16Spp, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
				// 1 spp
				.bindBuffer(3, &sobolInfo1Spp, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
				.bindBuffer(4, &rankingTileInfo1Spp, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
				.bindBuffer(5, &scramblingTileInfo1Spp, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
				// 4 spp
				.bindBuffer(6, &sobolInfo4Spp, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
				.bindBuffer(7, &rankingTileInfo4Spp, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
				.bindBuffer(8, &scramblingTileInfo4Spp, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
				.build(m_globalBlueNoise->set, m_globalBlueNoise->setLayouts);
		}

		return m_globalBlueNoise;
	}

	EpicAtmosphere* StaticTextures::getEpicAtmosphere()
	{
		if (m_epicAtmosphere == nullptr)
		{
			m_epicAtmosphere = new EpicAtmosphere();

			m_epicAtmosphere->multiScatterLut = RenderTexture::create(
				RHI::get()->getVulkanDevice(),
				EpicAtmosphere::MULTISCATTER_TEXTURE_WIDTH,
				EpicAtmosphere::MULTISCATTER_TEXTURE_HEIGHT,
				EpicAtmosphere::getMultiScatterLutFormat(),
				VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT
			);

			m_epicAtmosphere->multiScatterLut->transitionLayoutImmediately(
				RHI::get()->getGraphicsCommandPool(),
				RHI::get()->getGraphicsQueue(),
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT
			);

			m_epicAtmosphere->transmittanceLut = RenderTexture::create(
				RHI::get()->getVulkanDevice(),
				EpicAtmosphere::TRANSMITTANCE_TEXTURE_WIDTH,
				EpicAtmosphere::TRANSMITTANCE_TEXTURE_HEIGHT,
				EpicAtmosphere::getTransmittanceLutFormat(),
				VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT
			);

			m_epicAtmosphere->transmittanceLut->transitionLayoutImmediately(
				RHI::get()->getGraphicsCommandPool(),
				RHI::get()->getGraphicsQueue(),
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT
			);

			m_epicAtmosphere->skyViewLut = RenderTexture::create(
				RHI::get()->getVulkanDevice(),
				EpicAtmosphere::SKYVIEW_TEXTURE_WIDTH,
				EpicAtmosphere::SKYVIEW_TEXTURE_HEIGHT,
				EpicAtmosphere::getSkyViewLutFormat(),
				VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT
			);

			m_epicAtmosphere->skyViewLut->transitionLayoutImmediately(
				RHI::get()->getGraphicsCommandPool(),
				RHI::get()->getGraphicsQueue(),
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT
			);
		}

		return m_epicAtmosphere;
	}
	
}