#pragma once
#include "PassCommon.h"
#include "../SceneTextures.h"

namespace flower
{
	class HizPass : public GraphicsPass
	{
	public:
		HizPass(DefferedRenderer* renderer)
			: GraphicsPass(renderer, "HizPass")
		{

		}

		void dynamicRender(VkCommandBuffer cmd);

	private:
		bool bInitPipeline = false;

		virtual void initInner() override { createPipeline(); }
		virtual void onSceneTextureRecreate(uint32_t width, uint32_t height) override 
		{  
			destroyPipeline(); 
			createPipeline();
		}

		virtual void releaseInner() override { destroyPipeline(); }

		void createPipeline();
		void destroyPipeline();

	private:
		// Main
		VkPipeline m_pipelines = VK_NULL_HANDLE;
		VkPipelineLayout m_pipelineLayouts = VK_NULL_HANDLE;
		VulkanDescriptorSetReference m_descriptorSets = {};
		VulkanDescriptorLayoutReference m_descriptorSetLayouts = {};
	};
}