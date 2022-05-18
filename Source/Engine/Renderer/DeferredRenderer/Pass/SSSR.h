#pragma once
#include "PassCommon.h"
#include "../SceneTextures.h"

namespace flower
{
	class TextureManager;
	class SSSR : public GraphicsPass
	{
	public:
		SSSR(DefferedRenderer* renderer)
			: GraphicsPass(renderer, "StochasticScreenSpaceReflection")
		{

		}

		void dynamicRender(VkCommandBuffer cmd);

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

		// hiz intersect.
		VkPipeline m_pipelineIntersect = VK_NULL_HANDLE;

		// ssr resolve
		VkPipeline m_pipelineResolve = VK_NULL_HANDLE;

		// ssr temporal
		VkPipeline m_pipelineTemporal = VK_NULL_HANDLE;

		// ssr update
		VkPipeline m_pipelineUpdate = VK_NULL_HANDLE;
	};
}