#include "BRDFLut.h"

namespace flower
{
	void BRDFLutPass::update()
	{
		executeImmediately(RHI::get()->getDevice(), RHI::get()->getGraphicsCommandPool(), RHI::get()->getGraphicsQueue(), [this](VkCommandBuffer cmd) {

			m_renderer->getStaticTextures()->getBRDFLut()->transitionLayout(cmd, VK_IMAGE_LAYOUT_GENERAL);
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelines);

			std::vector<VkDescriptorSet> compPassSets = 
			{
				 m_descriptorSets.set
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

			vkCmdDispatch(cmd, StaticTextures::BRDF_LUT_DIM_XY / 16, StaticTextures::BRDF_LUT_DIM_XY / 16, 1);

			m_renderer->getStaticTextures()->getBRDFLut()->transitionLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		});
	}

	void BRDFLutPass::initInner()
	{
		createPipeline();
	}

	void BRDFLutPass::releaseInner()
	{
		destroyPipeline();
	}

	void BRDFLutPass::createPipeline()
	{
		VkDescriptorImageInfo lutImage = {};
		lutImage.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		lutImage.imageView = m_renderer->getStaticTextures()->getBRDFLut()->getImageView();;

		RHI::get()->vkDescriptorFactoryBegin()
			.bindImage(0, &lutImage, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
			.build(m_descriptorSets, m_descriptorSetLayouts);

		VkPipelineLayoutCreateInfo plci = vkPipelineLayoutCreateInfo();
		plci.pushConstantRangeCount = 0;

		std::vector<VkDescriptorSetLayout> setLayouts = 
		{
			m_descriptorSetLayouts.layout
		};

		plci.setLayoutCount = (uint32_t)setLayouts.size();
		plci.pSetLayouts = setLayouts.data();

		m_pipelineLayouts = RHI::get()->createPipelineLayout(plci);

		auto* shaderModule = RHI::get()->getShader("Shader/Spirv/BRDFLut.comp.spv");
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

	void BRDFLutPass::destroyPipeline()
	{
		vkDestroyPipeline(RHI::get()->getDevice(), m_pipelines, nullptr);
		vkDestroyPipelineLayout(RHI::get()->getDevice(), m_pipelineLayouts, nullptr);
	}
}

