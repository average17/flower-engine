#include "ImGuiPass.h"
#include "../Core/WindowData.h"

#include <ImGui/ImGui.h>
#include <ImGui/ImGuiGlfw.h>
#include <ImGui/ImGuiVulkan.h>

namespace flower
{
	static void checkVkResult(VkResult err)
	{
		if (err == 0)
			return;
		fprintf(stderr, "[imgui - vulkan] Error: VkResult = %d\n", err);
		if (err < 0)
			abort();
	}

	void setupVulkanInitInfo(ImGui_ImplVulkan_InitInfo* inout, VkDescriptorPool pool)
	{
		inout->Instance = RHI::get()->getInstance();
		inout->PhysicalDevice = RHI::get()->getVulkanDevice()->physicalDevice;
		inout->Device = RHI::get()->getDevice();
		inout->QueueFamily = RHI::get()->getVulkanDevice()->findQueueFamilies().graphicsFamily;
		inout->Queue = RHI::get()->getGraphicsQueue();
		inout->PipelineCache = VK_NULL_HANDLE;
		inout->DescriptorPool = pool;
		inout->Allocator = nullptr;
		inout->MinImageCount = (uint32_t)RHI::get()->getSwapchainImageViews().size();
		inout->ImageCount = inout->MinImageCount;
		inout->MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		inout->CheckVkResultFn = checkVkResult;
		inout->Subpass = 0;
	}

	void ImguiPass::renderpassBuild()
	{
		// Create the Render Pass
		{
			VkAttachmentDescription attachment = {};
			attachment.format = RHI::get()->getSwapchainFormat();
			attachment.samples = VK_SAMPLE_COUNT_1_BIT;
			attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			VkAttachmentReference color_attachment = {};
			color_attachment.attachment = 0;
			color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &color_attachment;

			VkSubpassDependency dependency = {};
			dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			dependency.dstSubpass = 0;
			dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.srcAccessMask = 0;
			dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			VkRenderPassCreateInfo info = {};

			info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			info.attachmentCount = 1;
			info.pAttachments = &attachment;
			info.subpassCount = 1;
			info.pSubpasses = &subpass;
			info.dependencyCount = 1;
			info.pDependencies = &dependency;
			vkCheck(vkCreateRenderPass(RHI::get()->getDevice(), &info, nullptr, &this->m_gpuResource.renderPass));
		}

		// Create Framebuffer & CommandBuffer
		{
			VkImageView attachment[1];
			VkFramebufferCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			info.renderPass = this->m_gpuResource.renderPass;
			info.attachmentCount = 1;
			info.pAttachments = attachment;
			info.width = RHI::get()->getSwapchainExtent().width;
			info.height = RHI::get()->getSwapchainExtent().height;
			info.layers = 1;

			auto backBufferSize = RHI::get()->getSwapchainImageViews().size();
			m_gpuResource.framebuffers.resize(backBufferSize);
			m_gpuResource.commandPools.resize(backBufferSize);
			m_gpuResource.commandBuffers.resize(backBufferSize);

			for (uint32_t i = 0; i < backBufferSize; i++)
			{
				attachment[0] = RHI::get()->getSwapchainImageViews()[i];
				vkCheck(vkCreateFramebuffer(RHI::get()->getDevice(), &info, nullptr, &m_gpuResource.framebuffers[i]));
			}

			for (uint32_t i = 0; i < backBufferSize; i++)
			{
				// Command pool
				{
					VkCommandPoolCreateInfo info = {};
					info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
					info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
					info.queueFamilyIndex = RHI::get()->getVulkanDevice()->findQueueFamilies().graphicsFamily;
					vkCheck(vkCreateCommandPool(RHI::get()->getDevice(), &info, nullptr, &m_gpuResource.commandPools[i]));
				}

				// Command buffer
				{
					VkCommandBufferAllocateInfo info = {};
					info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
					info.commandPool = m_gpuResource.commandPools[i];
					info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
					info.commandBufferCount = 1;
					vkCheck(vkAllocateCommandBuffers(RHI::get()->getDevice(), &info, &m_gpuResource.commandBuffers[i]));
				}
			}
		}
	}

	void ImguiPass::renderpassRelease()
	{
		vkDestroyRenderPass(RHI::get()->getDevice(),m_gpuResource.renderPass,nullptr);

		auto backBufferSize = m_gpuResource.framebuffers.size();
		for (uint32_t i = 0; i < backBufferSize; i++)
		{
			vkFreeCommandBuffers(RHI::get()->getDevice(),m_gpuResource.commandPools[i],1,&m_gpuResource.commandBuffers[i]);
			vkDestroyCommandPool(RHI::get()->getDevice(),m_gpuResource.commandPools[i],nullptr);
			vkDestroyFramebuffer(RHI::get()->getDevice(),m_gpuResource.framebuffers[i],nullptr);
		}

		m_gpuResource.framebuffers.resize(0);
		m_gpuResource.commandPools.resize(0);
		m_gpuResource.commandBuffers.resize(0);
	}

	void ImguiPass::init()
	{
		if(m_bInit)
		{
			return;
		}

		// Descriptor pool prepare.
		{
			VkDescriptorPoolSize pool_sizes[] =
			{
				{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
				{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
				{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
			};

			VkDescriptorPoolCreateInfo pool_info = {};
			pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
			pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
			pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
			pool_info.pPoolSizes = pool_sizes;
			vkCheck(vkCreateDescriptorPool(RHI::get()->getDevice(), &pool_info, nullptr, &m_gpuResource.descriptorPool));
		}

		renderpassBuild();

		// register swapchain rebuild functions.
		bool res0 = RHI::get()->onBeforeSwapchainRebuild.registerFunction(
			onBeforeSwapChainRebuildName,[&]() { renderpassRelease(); });
		bool res1 = RHI::get()->onAfterSwapchainRebuild.registerFunction(
			onAfterSwapChainRebuildName,[&]() { renderpassBuild(); });
		CHECK(res0 && res1 && "fail to register swapchain rebuild callbacks.");

		// init vulkan resource.
		ImGui_ImplVulkan_InitInfo vkInitInfo{ };
		setupVulkanInitInfo(&vkInitInfo,  m_gpuResource.descriptorPool);

		// init vulkan here.
		ImGui_ImplVulkan_Init(&vkInitInfo,m_gpuResource.renderPass);

		// upload font texture to gpu.
		{
			VkCommandPool command_pool = m_gpuResource.commandPools[0];
			VkCommandBuffer command_buffer = m_gpuResource.commandBuffers[0];
			vkCheck(vkResetCommandPool(RHI::get()->getDevice(), command_pool, 0));
			VkCommandBufferBeginInfo begin_info = {};
			begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			vkCheck(vkBeginCommandBuffer(command_buffer, &begin_info));
			ImGui_ImplVulkan_CreateFontsTexture(command_buffer);
			VkSubmitInfo end_info = {};
			end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			end_info.commandBufferCount = 1;
			end_info.pCommandBuffers = &command_buffer;
			vkCheck(vkEndCommandBuffer(command_buffer));
			vkCheck(vkQueueSubmit(vkInitInfo.Queue, 1, &end_info, VK_NULL_HANDLE));
			vkCheck(vkDeviceWaitIdle(vkInitInfo.Device));
			ImGui_ImplVulkan_DestroyFontUploadObjects();
		}

		m_bInit = true;
	}

	void ImguiPass::renderFrame(uint32_t backBufferIndex)
	{
		ImGui::Render();
		ImDrawData* main_draw_data = ImGui::GetDrawData();
		{
			vkCheck(vkResetCommandPool(RHI::get()->getDevice(), m_gpuResource.commandPools[backBufferIndex], 0));
			VkCommandBufferBeginInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			vkCheck(vkBeginCommandBuffer(m_gpuResource.commandBuffers[backBufferIndex], &info));
			setPerfMarkerBegin(m_gpuResource.commandBuffers[backBufferIndex], "ImGUI", {1.0f, 1.0f, 0.0f, 1.0f});
		}
		{
			VkRenderPassBeginInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			info.renderPass = m_gpuResource.renderPass;
			info.framebuffer = m_gpuResource.framebuffers[backBufferIndex];
			info.renderArea.extent.width = RHI::get()->getSwapchainExtent().width;
			info.renderArea.extent.height = RHI::get()->getSwapchainExtent().height;
			info.clearValueCount = 1;

			VkClearValue clearColor{ };
			clearColor.color.float32[0] = m_clearColor.x * m_clearColor.w;
			clearColor.color.float32[1] = m_clearColor.y * m_clearColor.w;
			clearColor.color.float32[2] = m_clearColor.z * m_clearColor.w;
			clearColor.color.float32[3] = m_clearColor.w;
			info.pClearValues = &clearColor;
			vkCmdBeginRenderPass(m_gpuResource.commandBuffers[backBufferIndex], &info, VK_SUBPASS_CONTENTS_INLINE);
		}

		ImGui_ImplVulkan_RenderDrawData(main_draw_data, m_gpuResource.commandBuffers[backBufferIndex]);

		vkCmdEndRenderPass(m_gpuResource.commandBuffers[backBufferIndex]);
		setPerfMarkerEnd(m_gpuResource.commandBuffers[backBufferIndex]);

		vkEndCommandBuffer(m_gpuResource.commandBuffers[backBufferIndex]);
	}

	void ImguiPass::release()
	{
		if(!m_bInit)
		{
			return;
		}

		// unregister swapchain rebuild functions.
		bool res0 = RHI::get()->onBeforeSwapchainRebuild.unregisterFunction(onBeforeSwapChainRebuildName);
		bool res1 = RHI::get()->onAfterSwapchainRebuild.unregisterFunction(onAfterSwapChainRebuildName);
		CHECK(res0 && res1 && "fail to unregister swapchain rebuild callbacks.");

		// shut down vulkan here.
		ImGui_ImplVulkan_Shutdown();

		renderpassRelease();
		vkDestroyDescriptorPool(RHI::get()->getDevice(), m_gpuResource.descriptorPool, nullptr);

		m_bInit = false;
	}
}