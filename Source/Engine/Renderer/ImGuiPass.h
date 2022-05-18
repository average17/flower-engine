#pragma once
#include "../RHI/RHI.h"

namespace flower
{
	class ImguiPass
	{
		struct ImguiPassGpuResource
		{
			VkDescriptorPool descriptorPool;
			VkRenderPass renderPass;

			std::vector<VkFramebuffer>   framebuffers;
			std::vector<VkCommandPool>   commandPools;
			std::vector<VkCommandBuffer> commandBuffers;
		}; 

	private:
		bool m_bInit = false;
		const char* onBeforeSwapChainRebuildName = "ImGuiPassBeforeSwapchainRebuild";
		const char* onAfterSwapChainRebuildName  = "ImGuiPassAfterSwapchainRebuild";
		ImguiPassGpuResource m_gpuResource;
		glm::vec4 m_clearColor { 0.45f, 0.55f, 0.60f, 1.00f};

	private:
		void renderpassBuild();
		void renderpassRelease();

	public:
		VkCommandBuffer getCommandBuffer(uint32_t index)
		{
			return m_gpuResource.commandBuffers[index];
		}

	public:
		~ImguiPass() = default;

		void init();
		void release();

		void renderFrame(uint32_t backBufferIndex);
	}; 
}