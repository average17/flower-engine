#pragma once
#include "PassCommon.h"
#include "../SceneTextures.h"

namespace flower
{
	class TonemapperPass : public GraphicsPass
	{
	public:
		TonemapperPass(DefferedRenderer* renderer)
			: GraphicsPass(renderer, "TonemapperPass")
		{

		}

		void dynamicRender(VkCommandBuffer cmd);

	private:
		VulkanDescriptorSetReference m_tonemapperPassDescriptorSet;
		VulkanDescriptorLayoutReference m_tonemapperPassDescriptorSetLayout;

		VkFramebuffer m_framebuffer = VK_NULL_HANDLE;
		VkRenderPass  m_renderpass  = VK_NULL_HANDLE;

		VkPipeline m_pipeline = VK_NULL_HANDLE;
		VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

		virtual void initInner() override;
		virtual void onSceneTextureRecreate(uint32_t width, uint32_t height) override;
		virtual void releaseInner() override;

	private:
		void createRenderpass();
		void releaseRenderpass();
		void createFramebuffer();
		void releaseFramebuffer();
		void createPipeline();
		void releasePipeline();
	};
}