#pragma once
#include "PassCommon.h"
#include "../SceneTextures.h"

namespace flower
{
	extern bool TAAOpen();

	class TAAPass : public GraphicsPass
	{
	public:
		TAAPass(DefferedRenderer* renderer)
			: GraphicsPass(renderer, "TAAPass")
		{

		}

		void dynamicRender(VkCommandBuffer cmd);

	private:
		bool bInitPipeline = false;

		virtual void initInner() override;
		virtual void onSceneTextureRecreate(uint32_t width, uint32_t height) override;
		virtual void releaseInner() override;

		void createPipeline();
		void destroyPipeline();
	private:
		// Main
		VkPipeline m_pipelines = VK_NULL_HANDLE;
		VkPipelineLayout m_pipelineLayouts = VK_NULL_HANDLE;
		VulkanDescriptorSetReference m_descriptorSets = {};
		VulkanDescriptorLayoutReference m_descriptorSetLayouts = {};

		// Sharpen
		VkPipeline m_pipelinesSharpen = VK_NULL_HANDLE;
		VkPipelineLayout m_pipelineLayoutsSharpen = VK_NULL_HANDLE;
		VulkanDescriptorSetReference m_descriptorSetsSharpen = {};
		VulkanDescriptorLayoutReference m_descriptorSetLayoutsSharpen = {};
	};
}