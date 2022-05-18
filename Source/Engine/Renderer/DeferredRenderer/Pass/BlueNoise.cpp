#include "BlueNoise.h"

namespace flower
{
	struct PushConstants
	{
		uint32_t frameIndex;
	};	

	void BlueNoisePass::dynamicRender(VkCommandBuffer cmd)
	{
		setPerfMarkerBegin(cmd, "BlueNoise", { 0.1f, 0.5f, 0.6f, 1.0f });
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelines);

		std::vector<VkDescriptorSet> compPassSets = {
			m_descriptorSets.set,
			m_renderer->getStaticTextures()->getGlobalBlueNoise()->set.set
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

		PushConstants pushConst{};
		constexpr uint64_t maxUint32t = 0xFFFFFFFF;
		uint64_t times = std::min(maxUint32t, m_renderer->getRenderTimes());

		pushConst.frameIndex = uint32_t(times);

		vkCmdPushConstants(cmd,
			m_pipelineLayouts,
			VK_SHADER_STAGE_COMPUTE_BIT, 0,
			sizeof(PushConstants),
			&pushConst
		);

		m_renderer->getStaticTextures()->getBlueNoise1Spp()->transitionLayout(cmd, VK_IMAGE_LAYOUT_GENERAL);

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelines);
		vkCmdDispatch(cmd, getGroupCount(StaticTextures::BLUE_NOISE_DIM_XY, 16), getGroupCount(StaticTextures::BLUE_NOISE_DIM_XY, 16), 1);
		m_renderer->getStaticTextures()->getBlueNoise1Spp()->transitionLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		setPerfMarkerEnd(cmd);
	}

	void BlueNoisePass::initInner()
	{
		createPipeline();
	}

	void BlueNoisePass::releaseInner()
	{
		destroyPipeline();
	}

	void BlueNoisePass::onSceneTextureRecreate(uint32_t width, uint32_t height)
	{
		destroyPipeline();
		createPipeline();
	}

	void BlueNoisePass::destroyPipeline()
	{
		if (m_pipelines != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(RHI::get()->getDevice(), m_pipelines, nullptr);
			vkDestroyPipelineLayout(RHI::get()->getDevice(), m_pipelineLayouts, nullptr);
			m_pipelines = VK_NULL_HANDLE;
		}
	}

	void BlueNoisePass::createPipeline()
	{
		{
			VkDescriptorImageInfo blueNoiseImage = {};
			blueNoiseImage.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			blueNoiseImage.imageView = m_renderer->getStaticTextures()->getBlueNoise1Spp()->getImageView();

			RHI::get()->vkDescriptorFactoryBegin()
				.bindImage(0, &blueNoiseImage, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
				.build(m_descriptorSets, m_descriptorSetLayouts);
		}

		{
			VkPipelineLayoutCreateInfo plci = vkPipelineLayoutCreateInfo();

			VkPushConstantRange pushConstantsRange{};
			pushConstantsRange.offset = 0;
			pushConstantsRange.size = sizeof(PushConstants);
			pushConstantsRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

			plci.pushConstantRangeCount = 1;
			plci.pPushConstantRanges = &pushConstantsRange;

			
			std::vector<VkDescriptorSetLayout> setLayouts =
			{
				m_descriptorSetLayouts.layout,
				m_renderer->getStaticTextures()->getGlobalBlueNoise()->setLayouts.layout
			};

			plci.setLayoutCount = (uint32_t)setLayouts.size();
			plci.pSetLayouts = setLayouts.data();

			m_pipelineLayouts = RHI::get()->createPipelineLayout(plci);

			auto* shaderModule = RHI::get()->getShader("Shader/Spirv/BlueNoise.comp.spv", true);

			VkPipelineShaderStageCreateInfo shaderStageCI{};
			shaderStageCI.module = shaderModule->GetModule();
			shaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStageCI.stage = VK_SHADER_STAGE_COMPUTE_BIT;
			shaderStageCI.pName = "main";

			VkComputePipelineCreateInfo computePipelineCreateInfo{};
			computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			computePipelineCreateInfo.layout = m_pipelineLayouts;
			computePipelineCreateInfo.flags = 0;
			computePipelineCreateInfo.stage = shaderStageCI;
			vkCheck(vkCreateComputePipelines(RHI::get()->getDevice(), nullptr, 1, &computePipelineCreateInfo, nullptr, &m_pipelines));
		}
	}

}