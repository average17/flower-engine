#pragma once

#include "PassCommon.h"
#include "../SceneTextures.h"
#include "../SceneStaticUniform.h"

namespace flower
{
	class TextureManager;
	class EpicAtmospherePass : public GraphicsPass
	{
	public:
		EpicAtmospherePass(DefferedRenderer* renderer)
			: GraphicsPass(renderer, "EpicAtmospherePass")
		{

		}

		void prepareSkyView(VkCommandBuffer cmd);
		void drawSky(VkCommandBuffer cmd);

	private:
		virtual void initInner() override { createPipeline(true); }
		virtual void onSceneTextureRecreate(uint32_t width, uint32_t height) override
		{
			destroyPipeline(false);
			createPipeline(false);
		}
		virtual void releaseInner() override { destroyPipeline(true); }

		void createPipeline(bool bFirstInit);
		void destroyPipeline(bool bFinalRelease);

	private:
		TextureManager* m_textureManager = nullptr;

		VulkanDescriptorSetReference m_descriptorSets = {};
		VulkanDescriptorLayoutReference m_descriptorSetLayouts = {};
		VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

		// NOTE: we replace area perspective with volumetric lighting.
		//       so don't compute here again.
		VkPipeline m_pipelineTransmittance = VK_NULL_HANDLE;
		VkPipeline m_pipelineMultiScatter = VK_NULL_HANDLE;
		VkPipeline m_pipelineSkyView = VK_NULL_HANDLE;
		VkPipeline m_pipelineDrawSky = VK_NULL_HANDLE;
	};
}