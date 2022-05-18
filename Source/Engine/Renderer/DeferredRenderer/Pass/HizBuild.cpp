#include "HizBuild.h"

namespace flower
{
	struct GpuPushConstant
	{
		glm::uvec4 dimXY;
		int32_t buildIndex;
	};

	void HizPass::dynamicRender(VkCommandBuffer cmd)
	{
		setPerfMarkerBegin(cmd, "HiZ", { 0.4f, 0.6f, 0.5f, 1.0f });
		{
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelines);

			std::vector<VkDescriptorSet> compPassSets = {
				m_descriptorSets.set,
			};

			vkCmdBindDescriptorSets(
				cmd,
				VK_PIPELINE_BIND_POINT_COMPUTE,
				m_pipelineLayouts,
				0,
				(uint32_t)compPassSets.size(),
				compPassSets.data(),
				0,
				nullptr
			);

			auto* hizTexture = m_renderer->getSceneTextures()->getHiz();

			hizTexture->transitionLayout(cmd, VK_IMAGE_LAYOUT_GENERAL);

			uint32_t startWidth = hizTexture->getExtent().width;
			uint32_t startHeight = hizTexture->getExtent().height;
			uint32_t preWidth = startWidth;
			uint32_t preHeight = startHeight;

			for (uint32_t i = 0; i < hizTexture->getInfo().mipLevels; i++)
			{
				GpuPushConstant pushConstant{};
				pushConstant.buildIndex = i;
				pushConstant.dimXY = { startWidth, startHeight, preWidth, preHeight };

				startWidth /= 2;
				startHeight /= 2;
				startWidth = glm::max(startWidth, 1u);
				startHeight = glm::max(startHeight, 1u);
				if (i != 0)
				{
					preWidth /= 2;
					preHeight /= 2;
					preHeight = glm::max(preHeight, 1u);
					preHeight = glm::max(preHeight, 1u);
				}
				
				vkCmdPushConstants(cmd, m_pipelineLayouts, VK_SHADER_STAGE_COMPUTE_BIT,
					0, sizeof(GpuPushConstant), &pushConstant);

				vkCmdDispatch(cmd, getGroupCount(pushConstant.dimXY.x, 16), getGroupCount(pushConstant.dimXY.y, 16), 1);

				if (i != hizTexture->getInfo().mipLevels - 1)
				{
					std::array<VkImageMemoryBarrier, 1> imageBarriers{};
					imageBarriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
					imageBarriers[0].image = hizTexture->getImage();
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
			}
			hizTexture->transitionLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
		setPerfMarkerEnd(cmd);
	}

	void HizPass::createPipeline()
	{
		auto* hizTexture = m_renderer->getSceneTextures()->getHiz();
		{
			// write.
			std::vector<VkDescriptorImageInfo> hizTextureMips;
			for (uint32_t i = 0; i < hizTexture->getInfo().mipLevels; ++i)
			{
				hizTextureMips.push_back(
					VkDescriptorImageInfo{
						VK_NULL_HANDLE,
						hizTexture->getMipmapView(i),
						VK_IMAGE_LAYOUT_GENERAL
					}
				);
			}

			VkDescriptorImageInfo depthImage = {};
			depthImage.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			depthImage.imageView = m_renderer->getSceneTextures()->getDepthStencil()->getDepthOnlyImageView();
			depthImage.sampler = RHI::get()->getPointClampEdgeSampler();

			// read.
			VkDescriptorImageInfo hizImage = {};
			hizImage.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			hizImage.imageView = hizTexture->getImageView();
			hizImage.sampler = RHI::get()->getPointClampEdgeSampler();

			RHI::get()->vkDescriptorFactoryBegin()
				.bindImages(
					0,
					(uint32_t)hizTextureMips.size(),
					hizTextureMips.data(),
					VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
					VK_SHADER_STAGE_COMPUTE_BIT)
				.bindImage(1, &depthImage, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
				.bindImage(2, &hizImage, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
				.build(m_descriptorSets, m_descriptorSetLayouts);
		}

		// create pipeline.
		{
			VkPipelineLayoutCreateInfo plci = vkPipelineLayoutCreateInfo();
			plci.pushConstantRangeCount = 1;
			VkPushConstantRange push_constant{};
			push_constant.offset = 0;
			push_constant.size = sizeof(GpuPushConstant);
			push_constant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			plci.pPushConstantRanges = &push_constant;

			std::vector<VkDescriptorSetLayout> setLayouts = {
				m_descriptorSetLayouts.layout
			};

			plci.setLayoutCount = (uint32_t)setLayouts.size();
			plci.pSetLayouts = setLayouts.data();

			m_pipelineLayouts = RHI::get()->createPipelineLayout(plci);

			const VkSpecializationMapEntry specializationMap = { 
				.constantID = 0, 
				.offset = 0,
				.size = sizeof(uint32_t) 
			};

			const uint32_t specializationData[] = { hizTexture->getInfo().mipLevels };

			const VkSpecializationInfo specializationInfo = { 1, &specializationMap, sizeof(specializationData), specializationData };

			auto* shaderModule = RHI::get()->getShader("Shader/Spirv/HizBuild.comp.spv");
			VkPipelineShaderStageCreateInfo shaderStageCI{};
			shaderStageCI.module = shaderModule->GetModule();
			shaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStageCI.stage = VK_SHADER_STAGE_COMPUTE_BIT;
			shaderStageCI.pName = "main";
			shaderStageCI.pSpecializationInfo = &specializationInfo;

			VkComputePipelineCreateInfo computePipelineCreateInfo{};
			computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			computePipelineCreateInfo.layout = m_pipelineLayouts;
			computePipelineCreateInfo.flags = 0;
			computePipelineCreateInfo.stage = shaderStageCI;
			vkCheck(vkCreateComputePipelines(RHI::get()->getDevice(), nullptr, 1, &computePipelineCreateInfo, nullptr, &m_pipelines));
		}
	}

	void HizPass::destroyPipeline()
	{
		vkDestroyPipeline(RHI::get()->getDevice(), m_pipelines, nullptr);
		vkDestroyPipelineLayout(RHI::get()->getDevice(), m_pipelineLayouts, nullptr);
	}

}