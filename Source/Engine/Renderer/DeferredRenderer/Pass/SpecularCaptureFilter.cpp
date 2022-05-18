#include "SpecularCaptureFilter.h"
#include "../../../Asset/AssetMesh.h"
#include "../../../Asset/AssetSystem.h"
#include "../../../Asset/AssetTexture.h"
#include "../../../Engine.h"

namespace flower
{
	struct GpuPushConstants
	{
		uint32_t envId;
		uint32_t samplerId; // use mipmap filter sampler when level bigger than #0.
		int32_t level;
		float roughness;
		float exposure;
	};

	void SpecularCaptureFilterPass::filterImage(RenderTextureCube* inCube, CombineTextureBindless* srcImage)
	{
		if (m_textureManager == nullptr)
		{
			m_textureManager = GEngine.getRuntimeModule<AssetSystem>()->getTextureManager();
		}

		VulkanDescriptorSetReference descriptorSets;

		std::vector<VkDescriptorImageInfo> envTextureMipTailDescriptors;
		for (uint32_t level = 0; level < inCube->getInfo().mipLevels; ++level)
		{
			envTextureMipTailDescriptors.push_back(
				VkDescriptorImageInfo{
					VK_NULL_HANDLE,
					inCube->getMipmapView(level),
					VK_IMAGE_LAYOUT_GENERAL
				}
			);
		}

		RHI::get()->vkDescriptorFactoryBegin()
			.bindImages(
				0,
				(uint32_t)envTextureMipTailDescriptors.size(),
				envTextureMipTailDescriptors.data(),
				VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				VK_SHADER_STAGE_COMPUTE_BIT)
			.build(descriptorSets, m_descriptorSetLayouts);

		if (m_pipelineLayouts == VK_NULL_HANDLE)
		{
			VkPipelineLayoutCreateInfo plci = vkPipelineLayoutCreateInfo();
			plci.pushConstantRangeCount = 1;
			VkPushConstantRange push_constant{};
			push_constant.offset = 0;
			push_constant.size = sizeof(GpuPushConstants);
			push_constant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			plci.pPushConstantRanges = &push_constant;

			std::vector<VkDescriptorSetLayout> setLayouts = {
				m_descriptorSetLayouts.layout,
				m_textureManager->getBindless()->getSetLayout(), 
				RHI::get()->getBindlessSampler()->getSetLayout(),
			};

			plci.setLayoutCount = (uint32_t)setLayouts.size();
			plci.pSetLayouts = setLayouts.data();

			m_pipelineLayouts = RHI::get()->createPipelineLayout(plci);
		}

		if(m_pipelines == VK_NULL_HANDLE)
		{
			const VkSpecializationMapEntry specializationMap = { 0, 0, sizeof(uint32_t) };
			const uint32_t specializationData[] = { inCube->getInfo().mipLevels };
			const VkSpecializationInfo specializationInfo = { 1, &specializationMap, sizeof(specializationData), specializationData };

			auto* shaderModule = RHI::get()->getShader("Shader/Spirv/SpecularCaptureMipGen.comp.spv");
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

		executeImmediately(RHI::get()->getDevice(), RHI::get()->getGraphicsCommandPool(), RHI::get()->getGraphicsQueue(), [&](VkCommandBuffer cmd) 
		{
			inCube->transitionLayout(cmd, VK_IMAGE_LAYOUT_GENERAL);

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelines);

			std::vector<VkDescriptorSet> compPassSets = {
				descriptorSets.set,
				m_textureManager->getBindless()->getSet(),
				RHI::get()->getBindlessSampler()->getSet(),
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

			GpuPushConstants pushConstants{};
			pushConstants.envId = srcImage->textureIndex;
			pushConstants.samplerId = srcImage->samplerIndex;
			pushConstants.exposure = 1.0f;

			CHECK(inCube->getExtent().width == inCube->getExtent().height);

			uint32_t kEnvMapSize = inCube->getExtent().width;
			uint32_t kEnvMapLevels = inCube->getInfo().mipLevels;

			const float deltaRoughness = 1.0f / std::max(float(kEnvMapLevels), 1.0f);
			for (uint32_t level = 0, size = kEnvMapSize; level < kEnvMapLevels; ++level, size /= 2) 
			{
				const uint32_t numGroups = std::max<uint32_t>(1, size / 8);

				pushConstants.level = level;
				pushConstants.roughness = level * deltaRoughness;

				vkCmdPushConstants(cmd, m_pipelineLayouts, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(GpuPushConstants), &pushConstants);
				vkCmdDispatch(cmd, numGroups, numGroups, 6);
			}

			inCube->transitionLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		});
	}

	void SpecularCaptureFilterPass::releaseInner()
	{
		if (m_pipelines != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(RHI::get()->getDevice(), m_pipelines, nullptr);
			m_pipelines = VK_NULL_HANDLE;
		}

		if (m_pipelineLayouts != VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(RHI::get()->getDevice(), m_pipelineLayouts, nullptr);
			m_pipelineLayouts = VK_NULL_HANDLE;
		}
	}
}

