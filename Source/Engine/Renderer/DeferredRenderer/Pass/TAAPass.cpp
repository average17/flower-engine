#include "TAAPass.h"

namespace flower
{
	static AutoCVarInt32 cVarTAAOpen(
		"r.AA.TAA",
		"Open TAA effect.",
		"AA",
		1,
		CVarFlags::ReadAndWrite | CVarFlags::RenderStateRelative
	);

	static AutoCVarInt32 cVarTAASharpenMode(
		"r.AA.TAA.SharpenMode",
		"TAA Sharpen Mode, 0 is off, 1 is responsive sharpen, 2 is cas sharpen.",
		"AA",
		2,
		CVarFlags::ReadAndWrite | CVarFlags::RenderStateRelative
	);

	static AutoCVarFloat cVarTAASharpeness(
		"r.AA.TAA.Sharpeness",
		"TAA Sharpeness, only valid on cas sharpen mode on.",
		"AA",
		0.7f,
		CVarFlags::ReadAndWrite | CVarFlags::RenderStateRelative
	);

	bool TAAOpen()
	{
		return cVarTAAOpen.get() != 0;
	}

	struct TAAPushConstant
	{
		uint32_t firstRender;
		uint32_t camMove;
	};

	struct TAASharpenPushConstant
	{
		uint32_t sharpenMethod;
		float sharpeness;
	};

	void TAAPass::dynamicRender(VkCommandBuffer cmd)
	{
		if (!TAAOpen()) return;

		setPerfMarkerBegin(cmd, "TAA", { 0.1f, 0.8f, 0.1f, 1.0f });

		setPerfMarkerBegin(cmd, "Main", { 0.1f, 0.5f, 0.1f, 1.0f });
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelines);
		TAAPushConstant pushConst{};
		pushConst.firstRender = m_renderer->getRenderTimes() > 1 ? 0 : 1;
		pushConst.camMove =
			m_renderer->getRenderScene()->getViewData().camViewProj ==
			m_renderer->getRenderScene()->getViewData().camViewProjLast ? 0 : 1;

		vkCmdPushConstants(cmd,
			m_pipelineLayouts,
			VK_SHADER_STAGE_COMPUTE_BIT, 0,
			sizeof(TAAPushConstant),
			&pushConst
		);

		std::vector<VkDescriptorSet> compPassSets = {
			m_renderer->getViewDataRing()->getSet().set,
			m_descriptorSets.set,
			m_renderer->getFrameDataRing()->getSet().set
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

		m_renderer->getSceneTextures()->getTAA()->transitionLayout(cmd, VK_IMAGE_LAYOUT_GENERAL);
		m_renderer->getSceneTextures()->getHistory()->transitionLayout(cmd, VK_IMAGE_LAYOUT_GENERAL);

		auto extent = m_renderer->getSceneTextures()->getHdrSceneColor()->getExtent();
		vkCmdDispatch(cmd, getGroupCount(extent.width, 16), getGroupCount(extent.height, 16), 1);
		setPerfMarkerEnd(cmd);

		setPerfMarkerBegin(cmd, "Sharpen", { 0.1f, 0.5f, 0.1f, 1.0f });
		std::array<VkImageMemoryBarrier, 2> imageBarriers{};
		imageBarriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageBarriers[0].image = m_renderer->getSceneTextures()->getTAA()->getImage();
		imageBarriers[0].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		imageBarriers[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		imageBarriers[0].subresourceRange.levelCount = 1;
		imageBarriers[0].subresourceRange.layerCount = 1;
		imageBarriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBarriers[0].oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		imageBarriers[0].newLayout = VK_IMAGE_LAYOUT_GENERAL;

		imageBarriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageBarriers[1].image = m_renderer->getSceneTextures()->getHistory()->getImage();
		imageBarriers[1].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		imageBarriers[1].dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		imageBarriers[1].subresourceRange.levelCount = 1;
		imageBarriers[1].subresourceRange.layerCount = 1;
		imageBarriers[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBarriers[1].oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		imageBarriers[1].newLayout = VK_IMAGE_LAYOUT_GENERAL;

		vkCmdPipelineBarrier(
			cmd,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			(uint32_t)imageBarriers.size(), imageBarriers.data()
		);

		m_renderer->getSceneTextures()->getHdrSceneColor()->transitionLayout(cmd, VK_IMAGE_LAYOUT_GENERAL);
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelinesSharpen);

		std::vector<VkDescriptorSet> compPassSetsSharpen = {
			m_descriptorSetsSharpen.set
		};

		vkCmdBindDescriptorSets(
			cmd,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			m_pipelineLayoutsSharpen,
			0,
			(uint32_t)compPassSetsSharpen.size(),
			compPassSetsSharpen.data(),
			0,
			nullptr
		);

		TAASharpenPushConstant sharpPushConst{};
		sharpPushConst.sharpenMethod = cVarTAASharpenMode.get();
		sharpPushConst.sharpeness = cVarTAASharpeness.get();
		vkCmdPushConstants(cmd,
			m_pipelineLayoutsSharpen,
			VK_SHADER_STAGE_COMPUTE_BIT, 0,
			sizeof(TAASharpenPushConstant),
			&sharpPushConst
		);

		vkCmdDispatch(cmd, getGroupCount(extent.width, 16), getGroupCount(extent.height, 16), 1);
		m_renderer->getSceneTextures()->getHdrSceneColor()->transitionLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		setPerfMarkerEnd(cmd);

		setPerfMarkerEnd(cmd);
	}

	void TAAPass::initInner()
	{
		if (TAAOpen())
		{
			createPipeline();
		}
	}

    void TAAPass::releaseInner()
    {
        destroyPipeline();
    }

    void TAAPass::onSceneTextureRecreate(uint32_t width, uint32_t height)
    {
        destroyPipeline();

		if (TAAOpen())
		{
			createPipeline();
		}
    }

	void TAAPass::destroyPipeline()
	{
		if (m_pipelines != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(RHI::get()->getDevice(), m_pipelines, nullptr);
			vkDestroyPipelineLayout(RHI::get()->getDevice(), m_pipelineLayouts, nullptr);

			vkDestroyPipeline(RHI::get()->getDevice(), m_pipelinesSharpen, nullptr);
			vkDestroyPipelineLayout(RHI::get()->getDevice(), m_pipelineLayoutsSharpen, nullptr);

			m_pipelines = VK_NULL_HANDLE;
		}
	}

	void TAAPass::createPipeline()
	{
		// TAA Main
		{
			VkDescriptorImageInfo outTAAImage = {};
			outTAAImage.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			outTAAImage.imageView = m_renderer->getSceneTextures()->getTAA()->getImageView();;

			VkDescriptorImageInfo depthImage = {};
			depthImage.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			depthImage.imageView = m_renderer->getSceneTextures()->getDepthStencil()->getDepthOnlyImageView();
			depthImage.sampler = RHI::get()->getLinearClampSampler();

			VkDescriptorImageInfo historyImage = {};
			historyImage.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			historyImage.imageView = m_renderer->getSceneTextures()->getHistory()->getImageView();
			historyImage.sampler = RHI::get()->getLinearClampSampler();

			VkDescriptorImageInfo velocityImage = {};
			velocityImage.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			velocityImage.imageView = m_renderer->getSceneTextures()->getVelocity()->getImageView();
			velocityImage.sampler = RHI::get()->getPointClampEdgeSampler();

			VkDescriptorImageInfo hdrImage = {};
			hdrImage.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			hdrImage.imageView = m_renderer->getSceneTextures()->getHdrSceneColor()->getImageView();
			hdrImage.sampler = RHI::get()->getLinearClampSampler();

			RHI::get()->vkDescriptorFactoryBegin()
				.bindImage(0, &outTAAImage, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
				.bindImage(1, &depthImage, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
				.bindImage(2, &historyImage, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
				.bindImage(3, &velocityImage, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
				.bindImage(4, &hdrImage, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
				.build(m_descriptorSets, m_descriptorSetLayouts);
		}

		// TAA Sharpen
		{
			VkDescriptorImageInfo inTAAImage = {};
			inTAAImage.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			inTAAImage.imageView = m_renderer->getSceneTextures()->getTAA()->getImageView();;
			inTAAImage.sampler = RHI::get()->getPointClampEdgeSampler();

			VkDescriptorImageInfo outHdrImage = {};
			outHdrImage.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			outHdrImage.imageView = m_renderer->getSceneTextures()->getHdrSceneColor()->getImageView();;
			outHdrImage.sampler = RHI::get()->getPointClampEdgeSampler();

			VkDescriptorImageInfo outHistoryImage = {};
			outHistoryImage.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			outHistoryImage.imageView = m_renderer->getSceneTextures()->getHistory()->getImageView();
			outHistoryImage.sampler = RHI::get()->getPointClampEdgeSampler();

			RHI::get()->vkDescriptorFactoryBegin()
				.bindImage(0, &outHdrImage, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
				.bindImage(1, &outHistoryImage, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
				.bindImage(2, &inTAAImage, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
				.build(m_descriptorSetsSharpen, m_descriptorSetLayoutsSharpen);
		}

		// TAA Main
		{
			VkPipelineLayoutCreateInfo plci = vkPipelineLayoutCreateInfo();

			plci.pushConstantRangeCount = 1;

			VkPushConstantRange push_constant{};
			push_constant.offset = 0;
			push_constant.size = sizeof(TAAPushConstant);
			push_constant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			plci.pPushConstantRanges = &push_constant;

			std::vector<VkDescriptorSetLayout> setLayouts = 
			{
				m_renderer->getViewDataRing()->getLayout().layout,
				m_descriptorSetLayouts.layout,
				m_renderer->getFrameDataRing()->getLayout().layout
			};

			plci.setLayoutCount = (uint32_t)setLayouts.size();
			plci.pSetLayouts = setLayouts.data();

			m_pipelineLayouts = RHI::get()->createPipelineLayout(plci);

			auto* shaderModule = RHI::get()->getShader("Shader/Spirv/TAAMain.comp.spv", true);

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

		// TAA Sharpen
		{
			VkPipelineLayoutCreateInfo plci = vkPipelineLayoutCreateInfo();
			plci.pushConstantRangeCount = 1;

			VkPushConstantRange push_constant{};
			push_constant.offset = 0;
			push_constant.size = sizeof(TAASharpenPushConstant);
			push_constant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			plci.pPushConstantRanges = &push_constant;

			std::vector<VkDescriptorSetLayout> setLayouts = {
				m_descriptorSetLayoutsSharpen.layout
			};

			plci.setLayoutCount = (uint32_t)setLayouts.size();
			plci.pSetLayouts = setLayouts.data();
			m_pipelineLayoutsSharpen = RHI::get()->createPipelineLayout(plci);

			auto* shaderModule = RHI::get()->getShader("Shader/Spirv/TAASharpen.comp.spv", true);
			VkPipelineShaderStageCreateInfo shaderStageCI{};
			shaderStageCI.module = shaderModule->GetModule();
			shaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStageCI.stage = VK_SHADER_STAGE_COMPUTE_BIT;
			shaderStageCI.pName = "main";

			VkComputePipelineCreateInfo computePipelineCreateInfo{};
			computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			computePipelineCreateInfo.layout = m_pipelineLayoutsSharpen;
			computePipelineCreateInfo.flags = 0;
			computePipelineCreateInfo.stage = shaderStageCI;
			vkCheck(vkCreateComputePipelines(RHI::get()->getDevice(), nullptr, 1, &computePipelineCreateInfo, nullptr, &m_pipelinesSharpen));
		}
	}

}