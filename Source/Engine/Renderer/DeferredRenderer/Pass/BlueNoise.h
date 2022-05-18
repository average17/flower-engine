#pragma once
#include "PassCommon.h"
#include "../SceneTextures.h"

namespace flower
{
	class BlueNoisePass : public GraphicsPass
	{
	public:
		BlueNoisePass(DefferedRenderer* renderer)
			: GraphicsPass(renderer, "BlueNoisePass")
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
		
		VkPipeline m_pipelines = VK_NULL_HANDLE;
		VkPipelineLayout m_pipelineLayouts = VK_NULL_HANDLE;
		VulkanDescriptorSetReference m_descriptorSets = {};
		VulkanDescriptorLayoutReference m_descriptorSetLayouts = {};
	};
}