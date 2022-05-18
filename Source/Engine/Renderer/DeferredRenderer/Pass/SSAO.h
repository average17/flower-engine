#pragma once
#include "PassCommon.h"
#include "../SceneTextures.h"

namespace flower
{
	class SSAO : public GraphicsPass
	{
	public:
		SSAO(DefferedRenderer* renderer)
			: GraphicsPass(renderer, "GroundTrueApproachAmbientOcclusion")
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
		VulkanDescriptorSetReference m_descriptorSets = {};
		VulkanDescriptorLayoutReference m_descriptorSetLayouts = {};
		VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

		// compute ao.
		VkPipeline m_pipelineComputeAO = VK_NULL_HANDLE;

		VulkanImage* m_ssaoTempFilter = nullptr;
		VulkanImage* m_ssaoTemporal = nullptr;

		VkPipeline m_pipelineFilterX = VK_NULL_HANDLE;
		VkPipeline m_pipelineFilterY = VK_NULL_HANDLE;

		VkPipeline m_pipelineTemporal = VK_NULL_HANDLE;
		VkPipeline m_pipelinePrevUpdate = VK_NULL_HANDLE;
	};
}