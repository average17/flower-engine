#pragma once
#include "PassCommon.h"
#include "../SceneStaticUniform.h"

namespace flower
{
	class CullingPass : public GraphicsPass
	{
	public:
		CullingPass(DefferedRenderer* renderer)
			: GraphicsPass(renderer, "CullingPass")
		{

		}

	private:
		VkPipeline m_pipeline = VK_NULL_HANDLE;
		VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

		VulkanDescriptorSetReference m_set;
		VulkanDescriptorLayoutReference m_setLayout;

		void createPipeline();
		void releasePipeline();

		virtual void initInner() override;
		virtual void onSceneTextureRecreate(uint32_t width, uint32_t height) override;
		virtual void releaseInner() override;

	public:
		void gbufferCulling(VkCommandBuffer cmd);
	};
}