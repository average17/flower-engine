#include "Bloom.h"


namespace flower
{
	static AutoCVarFloat cVarBloomThreshold(
		"r.Bloom.Threshold",
		"Bloom hard threshold",
		"Bloom",
		0.8f,
		CVarFlags::ReadAndWrite
	);

	static AutoCVarFloat cVarBloomSoftThreshold(
		"r.Bloom.ThresholdSoft",
		"Bloom solf threshold",
		"Bloom",
		0.5f,
		CVarFlags::ReadAndWrite
	);

	static AutoCVarFloat cVarBloomIntensity(
		"r.Bloom.Intensity",
		"Bloom intensity",
		"Bloom",
		1.1f,
		CVarFlags::ReadAndWrite
	);

	struct GpuBlurPushConstants
	{
		alignas(16) glm::vec2 direction;
		int32_t mipmapLevel;
	};

	struct GpuBlendPushConstants
	{
		float weight;
		float intensity;
	};

	struct DownSamplePushConstant
	{
		glm::vec4 filter;
		glm::vec2 invSize;
		glm::uvec2 size;
		int32_t mipLevel;
	};

	void Bloom::dynamicRender(VkCommandBuffer cmd)
	{
		setPerfMarkerBegin(cmd, "Bloom", { 1.0f, 0.6f, 1.0f, 1.0f });

		setPerfMarkerBegin(cmd, "DownSample", { 1.0f, 0.1f, 1.0f, 1.0f });
		// downsample.
		{
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_downSample.pipelines);

			{
				std::array<VkImageMemoryBarrier, 1> imageBarriers{};
				imageBarriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				imageBarriers[0].image = m_downSample.downsampleChainTexture->getImage();
				imageBarriers[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
				imageBarriers[0].dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
				imageBarriers[0].subresourceRange.baseMipLevel = 0; // We barrier mip level i
				imageBarriers[0].subresourceRange.levelCount = BLOOM_DOWNSAMPLE_COUNT;
				imageBarriers[0].subresourceRange.layerCount = 1;
				imageBarriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageBarriers[0].oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageBarriers[0].newLayout = VK_IMAGE_LAYOUT_GENERAL;
				vkCmdPipelineBarrier(
					cmd,
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					0,
					0, nullptr,
					0, nullptr,
					(uint32_t)imageBarriers.size(), imageBarriers.data()
);
			}

			DownSamplePushConstant pushData{};

			float knee = cVarBloomThreshold.get() * cVarBloomSoftThreshold.get();

			pushData.filter.x = cVarBloomThreshold.get();
			pushData.filter.y = pushData.filter.x - knee;
			pushData.filter.z = 2.0f * knee;
			pushData.filter.w = 0.25f / (knee + 0.00001f);

			uint32_t workingWidth = m_renderer->getSceneTextures()->getLdrSceneColor()->getInfo().extent.width;
			uint32_t workingHeight = m_renderer->getSceneTextures()->getLdrSceneColor()->getInfo().extent.height;

			for (uint32_t i = 0; i < BLOOM_DOWNSAMPLE_COUNT; i++)
			{
				pushData.mipLevel = i;

				workingWidth = std::max(1u, workingWidth / 2);
				workingHeight = std::max(1u, workingHeight / 2);

				pushData.invSize = glm::vec2(
					1.0f / float(workingWidth),
					1.0f / float(workingHeight)
				);
				pushData.size = glm::uvec2(
					workingWidth,
					workingHeight
				);

				vkCmdPushConstants(cmd, m_downSample.pipelineLayouts, VK_SHADER_STAGE_COMPUTE_BIT,
					0, sizeof(DownSamplePushConstant), &pushData);

				std::vector<VkDescriptorSet> compPassSets = {
					m_downSample.descriptorSets[i].set,
					m_downSample.inputDescritorSets[i].set
				};

				vkCmdBindDescriptorSets(
					cmd,
					VK_PIPELINE_BIND_POINT_COMPUTE,
					m_downSample.pipelineLayouts,
					0,
					(uint32_t)compPassSets.size(),
					compPassSets.data(),
					0,
					nullptr
				);

				vkCmdDispatch(cmd, getGroupCount(workingWidth, 16), getGroupCount(workingHeight, 16), 1);


				std::array<VkImageMemoryBarrier, 1> imageBarriers{};
				imageBarriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				imageBarriers[0].image = m_downSample.downsampleChainTexture->getImage();
				imageBarriers[0].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
				imageBarriers[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				imageBarriers[0].subresourceRange.baseMipLevel = i; // We barrier mip level i
				imageBarriers[0].subresourceRange.levelCount = 1;
				imageBarriers[0].subresourceRange.layerCount = 1;
				imageBarriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageBarriers[0].oldLayout = VK_IMAGE_LAYOUT_GENERAL;
				imageBarriers[0].newLayout = VK_IMAGE_LAYOUT_GENERAL;
				vkCmdPipelineBarrier(
					cmd,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					0,
					0, nullptr,
					0, nullptr,
					(uint32_t)imageBarriers.size(), imageBarriers.data()
				);
			}

			{
				std::array<VkImageMemoryBarrier, 1> imageBarriers{};
				imageBarriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				imageBarriers[0].image = m_downSample.downsampleChainTexture->getImage();
				imageBarriers[0].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
				imageBarriers[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				imageBarriers[0].subresourceRange.baseMipLevel = 0; // We barrier mip level i
				imageBarriers[0].subresourceRange.levelCount = BLOOM_DOWNSAMPLE_COUNT;
				imageBarriers[0].subresourceRange.layerCount = 1;
				imageBarriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageBarriers[0].oldLayout = VK_IMAGE_LAYOUT_GENERAL;
				imageBarriers[0].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				vkCmdPipelineBarrier(
					cmd,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					0,
					0, nullptr,
					0, nullptr,
					(uint32_t)imageBarriers.size(), imageBarriers.data()
				);
			}
		}
		setPerfMarkerEnd(cmd);

		setPerfMarkerBegin(cmd, "Upsacle with blur", { 1.0f, 0.2f, 1.0f, 1.0f });
		{
			CHECK(m_verticalBlur.size() == BLOOM_DOWNSAMPLE_COUNT);
			CHECK(m_verticalBlur.size() == m_horizontalBlur.size());

			uint32_t loopWidth = m_renderer->getSceneTextures()->getDepthStencil()->getExtent().width;
			uint32_t loopHeight = m_renderer->getSceneTextures()->getDepthStencil()->getExtent().height;

			uint32_t widthChain[BLOOM_DOWNSAMPLE_COUNT + 1]{ };
			uint32_t heightChain[BLOOM_DOWNSAMPLE_COUNT + 1]{ };

			for (auto i = 0; i < BLOOM_DOWNSAMPLE_COUNT + 1; i++)
			{
				widthChain[i] = loopWidth;
				heightChain[i] = loopHeight;

				loopWidth = loopWidth >> 1;
				loopHeight = loopHeight >> 1;
			}

			// from biggest mip to min mip.
			for (int32_t loopId = BLOOM_DOWNSAMPLE_COUNT - 1; loopId >= 0; loopId--)
			{
				// from mip #5 blur
				// then mix last blend mip.

				VkExtent2D blurExtent2D = { widthChain[loopId + 1], heightChain[loopId + 1] };
				CHECK(m_horizontalBlur[loopId]->getInfo().extent.width == widthChain[loopId + 1]);
				CHECK(m_horizontalBlur[loopId]->getInfo().extent.height == heightChain[loopId + 1]);

				// horizonral blur 
				{
					VkRenderPassBeginInfo rpInfo = vkRenderpassBeginInfo(
						m_renderpass,
						blurExtent2D,
						m_horizontalBlurFramebuffers[loopId]
					);
					VkClearValue colorValue;
					colorValue.color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
					VkClearValue depthClear;
					depthClear.depthStencil.depth = 1.f;
					depthClear.depthStencil.stencil = 1;
					VkClearValue clearValues[2] = { colorValue, depthClear };
					rpInfo.clearValueCount = 2;
					rpInfo.pClearValues = &clearValues[0];

					vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

					VkRect2D scissor{};
					scissor.extent = blurExtent2D;
					scissor.offset = { 0,0 };
					vkCmdSetScissor(cmd, 0, 1, &scissor);

					VkViewport viewport{};
					viewport.x = 0.0f;
					viewport.y = 0.0f;
					viewport.width = (float)widthChain[loopId + 1];
					viewport.height = (float)heightChain[loopId + 1];
					viewport.minDepth = 0.0f;
					viewport.maxDepth = 1.0f;

					vkCmdSetViewport(cmd, 0, 1, &viewport);
					vkCmdSetDepthBias(cmd, 0, 0, 0);

					vkCmdBindDescriptorSets(
						cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
						m_blurPipelineLayouts,
						0, // PassSet #0
						1,
						&m_horizontalBlurDescriptorSets[loopId].set, 0, nullptr
					);

					GpuBlurPushConstants blurConst{};
					// blur horizatonal dir
					blurConst.direction = glm::vec2(1.0f / (float)(widthChain[loopId + 1]), 0.0f);
					blurConst.mipmapLevel = loopId + 1;

					vkCmdPushConstants(cmd,
						m_blurPipelineLayouts,
						VK_SHADER_STAGE_FRAGMENT_BIT, 0,
						sizeof(GpuBlurPushConstants),
						&blurConst
					);

					vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_blurPipelines);
					vkCmdDraw(cmd, 3, 1, 0, 0);
					vkCmdEndRenderPass(cmd);
				}

				CHECK(m_verticalBlur[loopId]->getInfo().extent.width == widthChain[loopId + 1]);
				CHECK(m_verticalBlur[loopId]->getInfo().extent.height == heightChain[loopId + 1]);

				// vertical blur
				{
					VkRenderPassBeginInfo rpInfo = vkRenderpassBeginInfo(
						m_renderpass,
						blurExtent2D,
						m_verticalBlurFramebuffers[loopId]
					);
					VkClearValue colorValue;
					colorValue.color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
					VkClearValue depthClear;
					depthClear.depthStencil.depth = 1.f;
					depthClear.depthStencil.stencil = 1;
					VkClearValue clearValues[2] = { colorValue, depthClear };
					rpInfo.clearValueCount = 2;
					rpInfo.pClearValues = &clearValues[0];

					vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

					VkRect2D scissor{};
					scissor.extent = blurExtent2D;
					scissor.offset = { 0,0 };
					vkCmdSetScissor(cmd, 0, 1, &scissor);

					VkViewport viewport{};
					viewport.x = 0.0f;
					viewport.y = 0.0f;
					viewport.width = (float)widthChain[loopId + 1];
					viewport.height = (float)heightChain[loopId + 1];
					viewport.minDepth = 0.0f;
					viewport.maxDepth = 1.0f;

					vkCmdSetViewport(cmd, 0, 1, &viewport);
					vkCmdSetDepthBias(cmd, 0, 0, 0);

					vkCmdBindDescriptorSets(
						cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
						m_blurPipelineLayouts,
						0,
						1,
						&m_verticalBlurDescriptorSets[loopId].set, 0, nullptr
					);

					GpuBlurPushConstants blurConst{};

					// blur vertical direction
					blurConst.direction = glm::vec2(0.0f, 1.0f / (float)(heightChain[loopId + 1]));
					blurConst.mipmapLevel = loopId + 1;

					vkCmdPushConstants(cmd,
						m_blurPipelineLayouts,
						VK_SHADER_STAGE_FRAGMENT_BIT, 0,
						sizeof(GpuBlurPushConstants),
						&blurConst
					);

					vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_blurPipelines);
					vkCmdDraw(cmd, 3, 1, 0, 0);
					vkCmdEndRenderPass(cmd);
				}

				// upscale sample
				VkExtent2D blendExtent2D = { widthChain[loopId], heightChain[loopId] };

				// blend with last mip
				{
					VkRenderPassBeginInfo rpInfo = vkRenderpassBeginInfo(
						m_renderpass,
						blendExtent2D,
						m_blendFramebuffers[loopId]
					);
					VkClearValue colorValue;
					colorValue.color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
					VkClearValue depthClear;
					depthClear.depthStencil.depth = 1.f;
					depthClear.depthStencil.stencil = 1;
					VkClearValue clearValues[2] = { colorValue, depthClear };
					rpInfo.clearValueCount = 2;
					rpInfo.pClearValues = &clearValues[0];

					vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

					VkRect2D scissor{};
					scissor.extent = blendExtent2D;
					scissor.offset = { 0,0 };
					vkCmdSetScissor(cmd, 0, 1, &scissor);

					VkViewport viewport{};
					viewport.x = 0.0f;
					viewport.y = 0.0f;
					viewport.width = (float)widthChain[loopId];
					viewport.height = (float)heightChain[loopId];
					viewport.minDepth = 0.0f;
					viewport.maxDepth = 1.0f;

					vkCmdSetViewport(cmd, 0, 1, &viewport);
					vkCmdSetDepthBias(cmd, 0, 0, 0);

					vkCmdBindDescriptorSets(
						cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
						m_blendPipelineLayouts,
						0,
						1,
						&m_blendDescriptorSets[loopId].set, 0, nullptr
					);

					float blendConstants[4] = {
						m_blendWeight[loopId],
						m_blendWeight[loopId],
						m_blendWeight[loopId],
						m_blendWeight[loopId]
					};

					vkCmdSetBlendConstants(cmd, blendConstants);

					GpuBlendPushConstants blendConst{};
					blendConst.weight = loopId == 0 ? 1.0f - m_blendWeight[0] : 1.0f;
					blendConst.intensity = cVarBloomIntensity.get();
					vkCmdPushConstants(cmd,
						m_blendPipelineLayouts,
						VK_SHADER_STAGE_FRAGMENT_BIT, 0,
						sizeof(GpuBlendPushConstants),
						&blendConst
					);

					vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_blendPipelines);
					vkCmdDraw(cmd, 3, 1, 0, 0);
					vkCmdEndRenderPass(cmd);
				}
			}
		}
		setPerfMarkerEnd(cmd);

		setPerfMarkerEnd(cmd);
	}

	void Bloom::createPipeline()
	{
		m_downSample.init(this);

		// #0. create render pass.
		{
			VkAttachmentDescription colorAttachment = {};
			colorAttachment.format = SceneTextures::getHdrSceneColorFormat();
			colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachment.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkAttachmentReference colorAttachmentRef = {};
			colorAttachmentRef.attachment = 0;
			colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &colorAttachmentRef;
			subpass.pDepthStencilAttachment = nullptr;

			std::array<VkSubpassDependency, 2> dependencies;
			dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[0].dstSubpass = 0;
			dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
			dependencies[1].srcSubpass = 0;
			dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			VkRenderPassCreateInfo render_pass_info = {};
			render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			render_pass_info.attachmentCount = 1;
			render_pass_info.pAttachments = &colorAttachment;
			render_pass_info.subpassCount = 1;
			render_pass_info.pSubpasses = &subpass;
			render_pass_info.dependencyCount = 2;
			render_pass_info.pDependencies = dependencies.data();

			m_renderpass = RHI::get()->createRenderpass(render_pass_info);
		}


		// #1. create frame buffer.
		{
			// horizontal blur image
			{
				uint32_t width = m_renderer->getSceneTextures()->getHdrSceneColor()->getExtent().width;
				uint32_t height = m_renderer->getSceneTextures()->getHdrSceneColor()->getExtent().height;

				for (uint32_t index = 0; index < m_horizontalBlur.size(); index++)
				{
					/**
					**  #0 is mip1, #1 is mip2, #2 is mip3, #3 is mip4, #4 is mip5
					**  so just >> 1 to get actual size.
					*/

					width = width >> 1;
					height = height >> 1;

					// min scene texture size is (64u,64u) = (2^6, 2^6).
					// so there should always pass.
					CHECK(width >= 1);
					CHECK(height >= 1);

					CHECK(m_horizontalBlur[index] == nullptr);

					m_horizontalBlur[index] = RenderTexture::create(
						RHI::get()->getVulkanDevice(),
						width,
						height,
						SceneTextures::getHdrSceneColorFormat()
					);

					m_horizontalBlur[index]->transitionLayoutImmediately(
						RHI::get()->getGraphicsCommandPool(),
						RHI::get()->getGraphicsQueue(),
						VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
					);
				}
			}

			// vertical blur image
			{
				uint32_t width = m_renderer->getSceneTextures()->getHdrSceneColor()->getExtent().width;
				uint32_t height = m_renderer->getSceneTextures()->getHdrSceneColor()->getExtent().height;

				for (uint32_t index = 0; index < m_verticalBlur.size(); index++)
				{
					/**
					**  #0 is mip1, #1 is mip2, #2 is mip3, #3 is mip4, #4 is mip5
					**  so just >> 1 to get actual size.
					*/

					width = width >> 1;
					height = height >> 1;

					// min scene texture size is (64u,64u) = (2^6, 2^6).
					// so there should always pass.
					CHECK(width >= 1);
					CHECK(height >= 1);

					CHECK(m_verticalBlur[index] == nullptr);

					m_verticalBlur[index] = RenderTexture::create(
						RHI::get()->getVulkanDevice(),
						width,
						height,
						SceneTextures::getHdrSceneColorFormat()
					);

					m_verticalBlur[index]->transitionLayoutImmediately(
						RHI::get()->getGraphicsCommandPool(),
						RHI::get()->getGraphicsQueue(),
						VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
					);
				}
			}

			{
				// create horizontal framebuffer.
				{
					uint32_t width = m_renderer->getSceneTextures()->getHdrSceneColor()->getExtent().width;
					uint32_t height = m_renderer->getSceneTextures()->getHdrSceneColor()->getExtent().height;

					for (uint32_t i = 0; i < m_horizontalBlurFramebuffers.size(); i++)
					{
						/**
						**  #0 is mip1, #1 is mip2, #2 is mip3, #3 is mip4, #4 is mip5
						**  so just >> 1 to get actual size.
						*/

width = width >> 1;
height = height >> 1;

// min scene texture size is (64u,64u) = (2^6, 2^6).
// so there should always pass.
CHECK(width >= 1);
CHECK(height >= 1);

VkExtent2D extent2D{ width, height };

VulkanFrameBufferFactory fbf{};
fbf.setRenderpass(m_renderpass)
.addAttachment(
	m_horizontalBlur[i]->getImageView(), // we use horizontal blur image as rt.
	extent2D
);

m_horizontalBlurFramebuffers[i] = fbf.create(RHI::get()->getDevice());
					}
				}

				// create vertical framebuffer
				{
				uint32_t width = m_renderer->getSceneTextures()->getHdrSceneColor()->getExtent().width;
				uint32_t height = m_renderer->getSceneTextures()->getHdrSceneColor()->getExtent().height;

				for (uint32_t i = 0; i < m_verticalBlurFramebuffers.size(); i++)
				{
					/**
					**  #0 is mip1, #1 is mip2, #2 is mip3, #3 is mip4, #4 is mip5
					**  so just >> 1 to get actual size.
					*/

					width = width >> 1;
					height = height >> 1;

					// min scene texture size is (64u,64u) = (2^6, 2^6).
					// so there should always pass.
					CHECK(width >= 1);
					CHECK(height >= 1);

					VkExtent2D extent2D{ width, height };

					VulkanFrameBufferFactory fbf{};
					fbf.setRenderpass(m_renderpass)
						.addAttachment(
							m_verticalBlur[i]->getImageView(), // we use vertical blur image as rt.
							extent2D
						);

					m_verticalBlurFramebuffers[i] = fbf.create(RHI::get()->getDevice());
				}
				}

				// create blend framebuffer
				{
					uint32_t width = m_renderer->getSceneTextures()->getHdrSceneColor()->getExtent().width;
					uint32_t height = m_renderer->getSceneTextures()->getHdrSceneColor()->getExtent().height;

					for (uint32_t i = 0; i < BLOOM_DOWNSAMPLE_COUNT; i++)
					{
						//#0 is mip0, #1 is mip1, #2 is mip2, #3 is mip3, #4 is mip4 

						// min scene texture size is (64u,64u) = (2^6, 2^6).
						// so there should always pass.
						CHECK(width >= 1);
						CHECK(height >= 1);

						VkExtent2D extent2D{ width, height };

						VulkanFrameBufferFactory fbf{};

						if (i == 0)
						{
							fbf.setRenderpass(m_renderpass)
								.addAttachment(
									// we use hdr image as rt.
									m_renderer->getSceneTextures()->getHdrSceneColor()->getImageView(),
									extent2D
								);
						}
						else
						{
							fbf.setRenderpass(m_renderpass)
								.addAttachment(
									// we use bloom blur image as rt.
									m_downSample.downsampleChainTexture->getMipmapView(i - 1),
									extent2D
								);
						}

						m_blendFramebuffers[i] = fbf.create(RHI::get()->getDevice());

						width = width >> 1;
						height = height >> 1;
					}
				}
			}
		}

		
		{
			// create for horizontal blur 
			{
				for (uint32_t index = 0; index < m_horizontalBlurDescriptorSets.size(); index++)
				{
					VkDescriptorImageInfo horizontalBlurInputImage = {};
					horizontalBlurInputImage.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					horizontalBlurInputImage.sampler = RHI::get()->getLinearClampNoMipSampler();

					// horizatonal input mip #1 from down with blur blend
					//					 mip #2 from down with blur blend
					//					 mip #3 from down with blur blend
					//					 mip #4 from down with blur blend
					//					 mip #5 from down
					horizontalBlurInputImage.imageView = m_downSample.downsampleChainTexture->getMipmapView(index);

					// all bloom descriptor set layout same so just init one.
					if (index == 0)
					{
						RHI::get()->vkDescriptorFactoryBegin()
							.bindImage(0, &horizontalBlurInputImage, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
							.build(m_horizontalBlurDescriptorSets[index], m_descriptorSetLayout);
					}
					else
					{
						RHI::get()->vkDescriptorFactoryBegin()
							.bindImage(0, &horizontalBlurInputImage, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
							.build(m_horizontalBlurDescriptorSets[index]);
					}
				}
			}

			// create for vertical blur
			{
				for (uint32_t index = 0; index < m_verticalBlurDescriptorSets.size(); index++)
				{
					VkDescriptorImageInfo verticalBlurInputImage = {};
					verticalBlurInputImage.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					verticalBlurInputImage.sampler = RHI::get()->getLinearClampNoMipSampler();

					// vertical input horizontal blur mip #1
					//				  horizontal blur mip #2
					//				  horizontal blur mip #3
					//				  horizontal blur mip #4
					//				  horizontal blur mip #5
					verticalBlurInputImage.imageView = m_horizontalBlur[index]->getImageView();

					RHI::get()->vkDescriptorFactoryBegin()
						.bindImage(0, &verticalBlurInputImage, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
						.build(m_verticalBlurDescriptorSets[index]);
				}
			}

			// create for blend
			{
				for (uint32_t index = 0; index < m_blendDescriptorSets.size(); index++)
				{
					VkDescriptorImageInfo blendInputImage = {};
					blendInputImage.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					blendInputImage.sampler = RHI::get()->getLinearClampNoMipSampler();

					// blend input vertical blur mip #1
					//			   vertical blur mip #2
					//			   vertical blur mip #3
					//			   vertical blur mip #4
					//			   vertical blur mip #5
					blendInputImage.imageView = m_verticalBlur[index]->getImageView();

					RHI::get()->vkDescriptorFactoryBegin()
						.bindImage(0, &blendInputImage, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
						.build(m_blendDescriptorSets[index]);
				}
			}

			uint32_t swapchainCount = (uint32_t)RHI::get()->getSwapchainImageViews().size();

			// blur pipeline
			{
				{
					VkPipelineLayoutCreateInfo plci = vkPipelineLayoutCreateInfo();

					VkPushConstantRange push_constant{};
					push_constant.offset = 0;
					push_constant.size = sizeof(GpuBlurPushConstants);
					push_constant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
					plci.pPushConstantRanges = &push_constant;
					plci.pushConstantRangeCount = 1;

					std::vector<VkDescriptorSetLayout> setLayouts =
					{
						m_descriptorSetLayout.layout
					};

					plci.setLayoutCount = (uint32_t)setLayouts.size();
					plci.pSetLayouts = setLayouts.data();

					m_blurPipelineLayouts = RHI::get()->createPipelineLayout(plci);

					VulkanGraphicsPipelineFactory gpf = {};

					auto* vertShader = RHI::get()->getShader("Shader/Spirv/FullScreen.vert.spv");
					auto* fragShader = RHI::get()->getShader("Shader/Spirv/BloomBlur.frag.spv");

					gpf.shaderStages.clear();
					gpf.shaderStages.push_back(vkPipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, *vertShader));
					gpf.shaderStages.push_back(vkPipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, *fragShader));
					gpf.vertexInputInfo = vkVertexInputStateCreateInfo();

					gpf.vertexInputInfo.vertexBindingDescriptionCount = 0;
					gpf.vertexInputInfo.pVertexBindingDescriptions = nullptr;
					gpf.vertexInputInfo.vertexAttributeDescriptionCount = 0;
					gpf.vertexInputInfo.pVertexAttributeDescriptions = nullptr;

					gpf.inputAssembly = vkInputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
					gpf.depthStencil = vkDepthStencilCreateInfo(false, false, VK_COMPARE_OP_ALWAYS);

					gpf.rasterizer = vkRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);
					gpf.rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
					gpf.rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

					gpf.multisampling = vkMultisamplingStateCreateInfo();
					gpf.colorBlendAttachments = { vkColorBlendAttachmentState() };

					// we don't care about extent here. just use default.
					// we will set dynamic when we need.
					gpf.viewport.x = 0.0f;
					gpf.viewport.y = 0.0f;
					gpf.viewport.width = 10.0f;
					gpf.viewport.height = 10.0f;
					gpf.viewport.minDepth = 0.0f;
					gpf.viewport.maxDepth = 1.0f;
					gpf.scissor.offset = { 0, 0 };
					gpf.scissor.extent = { 10u,10u };

					gpf.pipelineLayout = m_blurPipelineLayouts;
					m_blurPipelines = gpf.buildMeshDrawPipeline(RHI::get()->getDevice(), m_renderpass);
				}
			}

			// blend pipeline
			{
				{
					VkPipelineLayoutCreateInfo plci = vkPipelineLayoutCreateInfo();

					VkPushConstantRange push_constant{};
					push_constant.offset = 0;
					push_constant.size = sizeof(GpuBlendPushConstants);
					push_constant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
					plci.pPushConstantRanges = &push_constant;
					plci.pushConstantRangeCount = 1;

					std::vector<VkDescriptorSetLayout> setLayouts =
					{
						m_descriptorSetLayout.layout
					};

					plci.setLayoutCount = (uint32_t)setLayouts.size();
					plci.pSetLayouts = setLayouts.data();

					m_blendPipelineLayouts = RHI::get()->createPipelineLayout(plci);

					VulkanGraphicsPipelineFactory gpf = {};

					auto* vertShader = RHI::get()->getShader("Shader/Spirv/FullScreen.vert.spv");
					auto* fragShader = RHI::get()->getShader("Shader/Spirv/BloomComposite.frag.spv");

					gpf.shaderStages.clear();
					gpf.shaderStages.push_back(vkPipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, *vertShader));
					gpf.shaderStages.push_back(vkPipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, *fragShader));
					gpf.vertexInputInfo = vkVertexInputStateCreateInfo();

					gpf.vertexInputInfo.vertexBindingDescriptionCount = 0;
					gpf.vertexInputInfo.pVertexBindingDescriptions = nullptr;
					gpf.vertexInputInfo.vertexAttributeDescriptionCount = 0;
					gpf.vertexInputInfo.pVertexAttributeDescriptions = nullptr;

					gpf.inputAssembly = vkInputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
					gpf.depthStencil = vkDepthStencilCreateInfo(false, false, VK_COMPARE_OP_ALWAYS);

					gpf.rasterizer = vkRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);
					gpf.rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
					gpf.rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

					gpf.multisampling = vkMultisamplingStateCreateInfo();

					// blend add.
					VkPipelineColorBlendAttachmentState att_state[1]{};
					att_state[0].colorWriteMask = 0xf;
					att_state[0].blendEnable = VK_TRUE;
					att_state[0].alphaBlendOp = VK_BLEND_OP_ADD;
					att_state[0].colorBlendOp = VK_BLEND_OP_ADD;
					att_state[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
					att_state[0].dstColorBlendFactor = VK_BLEND_FACTOR_CONSTANT_COLOR;
					att_state[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
					att_state[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;

					VkPipelineColorBlendStateCreateInfo cb{};
					cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
					cb.flags = 0;
					cb.pNext = NULL;
					cb.attachmentCount = 1;
					cb.pAttachments = att_state;
					cb.logicOpEnable = VK_FALSE;
					cb.logicOp = VK_LOGIC_OP_NO_OP;
					cb.blendConstants[0] = 1.0f;
					cb.blendConstants[1] = 1.0f;
					cb.blendConstants[2] = 1.0f;
					cb.blendConstants[3] = 1.0f;

					gpf.colorBlendAttachments = { att_state[0] };

					// we don't care about extent here. just use default.
					// we will set dynamic when we need.
					gpf.viewport.x = 0.0f;
					gpf.viewport.y = 0.0f;
					gpf.viewport.width = 10.0f;
					gpf.viewport.height = 10.0f;
					gpf.viewport.minDepth = 0.0f;
					gpf.viewport.maxDepth = 1.0f;
					gpf.scissor.offset = { 0, 0 };
					gpf.scissor.extent = { 10u,10u };

					gpf.pipelineLayout = m_blendPipelineLayouts;
					m_blendPipelines = gpf.buildMeshDrawPipeline(RHI::get()->getDevice(), m_renderpass, cb);
				}
			}
		}
	}

	void Bloom::destroyPipeline()
	{
		m_downSample.release();

		RHI::get()->destroyRenderpass(m_renderpass);

		// destroy image first
		{
			for (uint32_t i = 0; i < m_horizontalBlur.size(); i++)
			{
				delete m_horizontalBlur[i];
				m_horizontalBlur[i] = nullptr;
			}

			for (uint32_t i = 0; i < m_verticalBlur.size(); i++)
			{
				delete m_verticalBlur[i];
				m_verticalBlur[i] = nullptr;
			}
		}

		// then destroy frame buffers
		{
			for (uint32_t i = 0; i < m_horizontalBlurFramebuffers.size(); i++)
			{
				RHI::get()->destroyFramebuffer(m_horizontalBlurFramebuffers[i]);
				m_horizontalBlurFramebuffers[i] = VK_NULL_HANDLE;
			}

			for (uint32_t i = 0; i < m_verticalBlurFramebuffers.size(); i++)
			{
				RHI::get()->destroyFramebuffer(m_verticalBlurFramebuffers[i]);
				m_verticalBlurFramebuffers[i] = VK_NULL_HANDLE;
			}

			for (uint32_t i = 0; i < m_blendFramebuffers.size(); i++)
			{
				RHI::get()->destroyFramebuffer(m_blendFramebuffers[i]);
				m_blendFramebuffers[i] = VK_NULL_HANDLE;
			}
		}

		vkDestroyPipeline(RHI::get()->getDevice(), m_blurPipelines, nullptr);
		vkDestroyPipelineLayout(RHI::get()->getDevice(), m_blurPipelineLayouts, nullptr);

		vkDestroyPipeline(RHI::get()->getDevice(), m_blendPipelines, nullptr);
		vkDestroyPipelineLayout(RHI::get()->getDevice(), m_blendPipelineLayouts, nullptr);
	}

	void Bloom::DownSample::init(Bloom* in)
	{
		CHECK(downsampleChainTexture == nullptr);
		downsampleChainTexture = RenderTexture::create(
			RHI::get()->getVulkanDevice(),
			std::max(1u, in->getRenderer()->getRenderWidth() / 2), 
			std::max(1u, in->getRenderer()->getRenderHeight() / 2),
			BLOOM_DOWNSAMPLE_COUNT,
			true,
			SceneTextures::getHdrSceneColorFormat(),
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT
		);

		downsampleChainTexture->transitionLayoutImmediately(RHI::get()->getGraphicsCommandPool(), RHI::get()->getGraphicsQueue(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);


		descriptorSets.resize(BLOOM_DOWNSAMPLE_COUNT);
		inputDescritorSets.resize(BLOOM_DOWNSAMPLE_COUNT);
		{
			// for output
			for (uint32_t level = 0; level < downsampleChainTexture->getInfo().mipLevels; ++level)
			{
				VkDescriptorImageInfo outImage = {};
				outImage.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
				outImage.imageView = downsampleChainTexture->getMipmapView(level);
				outImage.sampler = RHI::get()->getLinearClampNoMipSampler();

				if (level == 0)
				{
					RHI::get()->vkDescriptorFactoryBegin()
						.bindImage(
							0,
							&outImage,
							VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
							VK_SHADER_STAGE_COMPUTE_BIT)
						.build(descriptorSets[level], descriptorSetLayouts);
				}
				else
				{
					RHI::get()->vkDescriptorFactoryBegin()
						.bindImage(
							0,
							&outImage,
							VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
							VK_SHADER_STAGE_COMPUTE_BIT)
						.build(descriptorSets[level]);
				}
			}

			// for input
			VkDescriptorImageInfo hdrImage = {};
			hdrImage.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			hdrImage.imageView = in->getRenderer()->getSceneTextures()->getHdrSceneColor()->getImageView();
			hdrImage.sampler = RHI::get()->getLinearClampNoMipSampler();
			RHI::get()->vkDescriptorFactoryBegin()
				.bindImage(
					0,
					&hdrImage,
					VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					VK_SHADER_STAGE_COMPUTE_BIT)
				.build(inputDescritorSets[0], inputDescriptorSetLayouts);


			for (uint32_t level = 0; level < downsampleChainTexture->getInfo().mipLevels - 1; ++level)
			{
				VkDescriptorImageInfo inputChainImage = {};
				inputChainImage.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
				inputChainImage.imageView = downsampleChainTexture->getMipmapView(level);
				inputChainImage.sampler = RHI::get()->getLinearClampNoMipSampler();

				RHI::get()->vkDescriptorFactoryBegin()
					.bindImage(
						0,
						&inputChainImage,
						VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
						VK_SHADER_STAGE_COMPUTE_BIT)
					.build(inputDescritorSets[level + 1]);
			}
		}

		// pipeline create.
		{
			VkPipelineLayoutCreateInfo plci = vkPipelineLayoutCreateInfo();
			plci.pushConstantRangeCount = 1;

			VkPushConstantRange push_constant{};
			push_constant.offset = 0;
			push_constant.size = sizeof(DownSamplePushConstant);
			push_constant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			plci.pPushConstantRanges = &push_constant;

			std::vector<VkDescriptorSetLayout> setLayouts = {
				descriptorSetLayouts.layout,
				inputDescriptorSetLayouts.layout
			};

			plci.setLayoutCount = (uint32_t)setLayouts.size();
			plci.pSetLayouts = setLayouts.data();

			pipelineLayouts = RHI::get()->createPipelineLayout(plci);

			const VkSpecializationMapEntry specializationMap = { 0, 0, sizeof(uint32_t) };
			const uint32_t specializationData[] = { BLOOM_DOWNSAMPLE_COUNT };
			const VkSpecializationInfo specializationInfo = { 1, &specializationMap, sizeof(specializationData), specializationData };

			auto* shaderModule = RHI::get()->getShader("Shader/Spirv/BloomDownsample.comp.spv", true);
			VkPipelineShaderStageCreateInfo shaderStageCI{};
			shaderStageCI.module = shaderModule->GetModule();
			shaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStageCI.stage = VK_SHADER_STAGE_COMPUTE_BIT;
			shaderStageCI.pName = "main";
			shaderStageCI.pSpecializationInfo = &specializationInfo;

			VkComputePipelineCreateInfo computePipelineCreateInfo{};
			computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			computePipelineCreateInfo.layout = pipelineLayouts;
			computePipelineCreateInfo.flags = 0;
			computePipelineCreateInfo.stage = shaderStageCI;
			vkCheck(vkCreateComputePipelines(RHI::get()->getDevice(), nullptr, 1, &computePipelineCreateInfo, nullptr, &pipelines));
		}
	}

	void Bloom::DownSample::release()
	{
		if (downsampleChainTexture)
		{
			delete downsampleChainTexture;
			downsampleChainTexture = nullptr;

			vkDestroyPipeline(RHI::get()->getDevice(), pipelines, nullptr);
			vkDestroyPipelineLayout(RHI::get()->getDevice(), pipelineLayouts, nullptr);
		}

	}


}