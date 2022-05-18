#pragma once
#include "PassCommon.h"
#include "../SceneTextures.h"

namespace flower
{
	class BRDFLutPass : public GraphicsPass
	{
	public:
		BRDFLutPass(DefferedRenderer* renderer)
			: GraphicsPass(renderer, "BRDFLutPass")
		{

		}

		void update();

	private:
		virtual void initInner() override;
		virtual void onSceneTextureRecreate(uint32_t width, uint32_t height) override {}
		virtual void releaseInner() override;

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