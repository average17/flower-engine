#pragma once
#include "PassCommon.h"
#include "../SceneStaticUniform.h"

namespace flower
{
	constexpr uint32_t BLOOM_DOWNSAMPLE_COUNT = 5u;

	class Bloom : public GraphicsPass
	{
		struct DownSample
		{
			RenderTexture* downsampleChainTexture = nullptr;

			void init(Bloom* in);
			void release();

			VkPipeline pipelines = {};
			VkPipelineLayout pipelineLayouts = {};

			std::vector<VulkanDescriptorSetReference> descriptorSets = {};
			VulkanDescriptorLayoutReference descriptorSetLayouts = {};

			std::vector<VulkanDescriptorSetReference> inputDescritorSets = {};
			VulkanDescriptorLayoutReference inputDescriptorSetLayouts = {};
		};


	public:
		Bloom(DefferedRenderer* renderer)
			: GraphicsPass(renderer, "Bloom")
		{

		}

		void dynamicRender(VkCommandBuffer cmd);

	private:
		DownSample m_downSample;

		// horizontal blur image result.
		// mip 5. 4. 3. 2. 1.
		std::array<VulkanImage*, BLOOM_DOWNSAMPLE_COUNT> m_horizontalBlur{};

		// vertical blur image result.
		// mip 5. 4. 3. 2. 1.
		std::array<VulkanImage*, BLOOM_DOWNSAMPLE_COUNT> m_verticalBlur{};

		std::array<float, BLOOM_DOWNSAMPLE_COUNT + 1> m_blendWeight = {
			1.0f - 0.08f, // 0
			0.25f,        // 1
			0.75f,        // 2
			1.5f,         // 3
			2.5f,         // 4
			3.0f          // 5
		};

		VulkanDescriptorLayoutReference m_descriptorSetLayout{};

		VkPipeline m_blurPipelines = {};
		VkPipelineLayout m_blurPipelineLayouts = {};
		
		std::array<VulkanDescriptorSetReference, BLOOM_DOWNSAMPLE_COUNT> m_horizontalBlurDescriptorSets = {}; // 
		std::array<VulkanDescriptorSetReference, BLOOM_DOWNSAMPLE_COUNT> m_verticalBlurDescriptorSets = {};

		VkPipeline m_blendPipelines = {};
		VkPipelineLayout m_blendPipelineLayouts = {};
		std::array<VulkanDescriptorSetReference, BLOOM_DOWNSAMPLE_COUNT> m_blendDescriptorSets = {};


		VkRenderPass m_renderpass;
		std::array<VkFramebuffer, BLOOM_DOWNSAMPLE_COUNT> m_verticalBlurFramebuffers = {};
		std::array<VkFramebuffer, BLOOM_DOWNSAMPLE_COUNT> m_horizontalBlurFramebuffers = {};
		std::array<VkFramebuffer, BLOOM_DOWNSAMPLE_COUNT> m_blendFramebuffers = {};

	private:
		virtual void initInner() override { createPipeline(); }
		virtual void onSceneTextureRecreate(uint32_t width, uint32_t height) override
		{
			destroyPipeline();
			createPipeline();
		}
		virtual void releaseInner() override { destroyPipeline(); }

		void createPipeline();
		void destroyPipeline();
	};
}