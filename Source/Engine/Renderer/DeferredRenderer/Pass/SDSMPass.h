#pragma once

#include "PassCommon.h"

namespace flower
{
	// traditional sdsm:
	// #0. get min max of depth.
	// #1. prepare cascade info by pass#0's result.
	// #2. build draw call.
	// #3. draw.

	// instance merge sdsm:
	// #0 & #1 same with traditional.
	// #2. build draw call with instance merge.
	// #3. one pass draw instance on rt dimXY, but atomic min draw depth on uav physical page.

	extern uint32_t getShadowPerPageDimXY();
	extern uint32_t getShadowCascadeCount();

	class SDSMRenderResource
	{
	private:
		bool m_bInitFlag = false;

	public:
		// Common buffer.
		SingleSSBO<GPUDepthRange, 1> depthRangeBuffer;
		SingleSSBO<GPUCascadeInfo, MAX_CACSADE_COUNT> cascadeInfoBuffer;

		using CascadeIndirectBuffer = DrawIndirectBuffer<GPUDrawCallData, MAX_CASCADE_SSBO_OBJECTS, GPUOutIndirectDrawCount, MAX_CACSADE_COUNT>;

		CascadeIndirectBuffer indirectCasccadeBuffer;

	public:
		void init();
		void release();
	};

	extern SDSMRenderResource GSDSMRenderResource;

	class TextureManager;
	class MeshManager;
	class SDSMPass : public GraphicsPass
	{
		struct DepthMinMaxEvaluate
		{
			VkPipeline pipeline = VK_NULL_HANDLE;
			VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
			VulkanDescriptorSetReference set;
			VulkanDescriptorLayoutReference setLayout;

			void init(SDSMPass* in);
			void release();
		};
		DepthMinMaxEvaluate m_depthMinMaxEvaluate;

		struct CascadeSetup
		{
			VkPipeline pipeline = VK_NULL_HANDLE;
			VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

			VulkanDescriptorSetReference set;
			VulkanDescriptorLayoutReference setLayout;

			void init(SDSMPass* in);
			void release();
		};
		CascadeSetup m_cascadeSetup;

		struct CascadeCulling
		{
			VkPipeline pipeline = VK_NULL_HANDLE;
			VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

			void init(SDSMPass* in);
			void release();
		};
		CascadeCulling m_cascadeCulling;

		struct CascadeDepth
		{
			VkPipeline pipeline = VK_NULL_HANDLE;
			VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

			VkRenderPass renderpass = VK_NULL_HANDLE;
			VkFramebuffer framebuffer = VK_NULL_HANDLE;

			void init(SDSMPass* in);
			void release();
		};
		CascadeDepth m_cascadeDepth;

	public:
		SDSMPass(DefferedRenderer* renderer)
			: GraphicsPass(renderer, "SDSMPass")
		{

		}
		void dynamicRecord(VkCommandBuffer cmd);


	private:
		virtual void initInner() override;
		virtual void onSceneTextureRecreate(uint32_t width, uint32_t height) override;
		virtual void releaseInner() override;

		MeshManager* m_meshManager;
		TextureManager* m_textureManager;
	};
}