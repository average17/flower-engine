#include "SSAO.h"
#include "../../../Asset/AssetSystem.h"
#include "../../../Engine.h"

namespace flower
{
	struct PushConstants
	{
		int32_t aoCut;
		int32_t aoDirection;
		int32_t aoSteps;
		float   aoRadius;
		float   soStrength;
		float   aoPower;
	};

	static AutoCVarFloat cVarSSAORadius(
		"r.SSAO.Radius",
		"SSAO radius.",
		"SSAO",
		2.0f,
		CVarFlags::ReadAndWrite
	);

	static AutoCVarInt32 cVarSSAOSteps(
		"r.SSAO.Steps",
		"ssao steps.",
		"SSAO",
		4,
		CVarFlags::ReadAndWrite
	);

	static AutoCVarInt32 cVarSSAODirection(
		"r.SSAO.Direction",
		"ssao direction.",
		"SSAO",
		2,
		CVarFlags::ReadAndWrite
	);

	static AutoCVarFloat cVarSSAOSOStrength(
		"r.SSAO.SOStrength",
		"SSAO SO Strength.",
		"SSAO",
		0.2f,
		CVarFlags::ReadAndWrite
	);

	static AutoCVarFloat cVarSSAOAOPower(
		"r.SSAO.AOPower",
		"SSAO AO Power.",
		"SSAO",
		1.0f,
		CVarFlags::ReadAndWrite
	);

	void SSAO::dynamicRender(VkCommandBuffer cmd)
	{
		if (m_renderer->getRenderTimes() < 2) { return; }

		setPerfMarkerBegin(cmd, "SSAO", { 0.5f, 0.5f, 0.3f, 1.0f });

		uint32_t fullRenderWidth = m_renderer->getRenderWidth();
		uint32_t fullRenderHeight = m_renderer->getRenderHeight();

		PushConstants pushConst{};
		pushConst.aoDirection = (uint32_t)cVarSSAODirection.get();
		pushConst.aoSteps = (uint32_t)cVarSSAOSteps.get();
		pushConst.aoRadius = cVarSSAORadius.get();
		pushConst.soStrength = cVarSSAOSOStrength.get();
		pushConst.aoPower = cVarSSAOAOPower.get();

		const auto renderTimes = m_renderer->getRenderTimes();
		pushConst.aoCut = renderTimes != 0 ? 1 : 0;

		auto bindSet = [this, &cmd, &pushConst]() {
			std::vector<VkDescriptorSet> compPassSets = {
				m_renderer->getFrameDataRing()->getSet().set,
				m_renderer->getViewDataRing()->getSet().set,
				m_descriptorSets.set,
				m_renderer->getStaticTextures()->getGlobalBlueNoise()->set.set
			};

			vkCmdBindDescriptorSets(
				cmd,
				VK_PIPELINE_BIND_POINT_COMPUTE,
				m_pipelineLayout,
				0,
				(uint32_t)compPassSets.size(),
				compPassSets.data(),
				0,
				nullptr
			);

			vkCmdPushConstants(cmd,
				m_pipelineLayout,
				VK_SHADER_STAGE_COMPUTE_BIT, 0,
				sizeof(PushConstants),
				&pushConst
			);
		};

		bindSet();
		auto downSize = SceneTextures::SSAODownScaleSize();
		setPerfMarkerBegin(cmd, "SSAO-Compute", { 0.9f, 0.5f, 0.9f, 1.0f });
		{
			m_renderer->getSceneTextures()->getSSAOMask()->transitionLayout(cmd, VK_IMAGE_LAYOUT_GENERAL);

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineComputeAO);
			vkCmdDispatch(cmd, getGroupCount(fullRenderWidth / downSize, 16), getGroupCount(fullRenderHeight / downSize, 16), 1);

			m_renderer->getSceneTextures()->getSSAOMask()->transitionLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
		setPerfMarkerEnd(cmd);

		setPerfMarkerBegin(cmd, "SSAO-Temporal", { 0.9f, 0.5f, 0.9f, 1.0f });
		{
			m_ssaoTempFilter->transitionLayout(cmd, VK_IMAGE_LAYOUT_GENERAL);

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineTemporal);
			vkCmdDispatch(cmd, getGroupCount(fullRenderWidth / downSize, 16), getGroupCount(fullRenderHeight / downSize, 16), 1);

			m_ssaoTempFilter->transitionLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
		setPerfMarkerEnd(cmd);

		setPerfMarkerBegin(cmd, "SSAO-UpdatePrev", { 0.9f, 0.5f, 0.9f, 1.0f });
		{
			m_ssaoTemporal->transitionLayout(cmd, VK_IMAGE_LAYOUT_GENERAL);

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelinePrevUpdate);
			vkCmdDispatch(cmd, getGroupCount(fullRenderWidth / downSize, 16), getGroupCount(fullRenderHeight / downSize, 16), 1);

			m_ssaoTemporal->transitionLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
		setPerfMarkerEnd(cmd);

		setPerfMarkerBegin(cmd, "SSAO-FilterX", { 0.9f, 0.5f, 0.9f, 1.0f });
		{
			m_ssaoTempFilter->transitionLayout(cmd, VK_IMAGE_LAYOUT_GENERAL);

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineFilterX);
			vkCmdDispatch(cmd, getGroupCount(fullRenderWidth / downSize, 16), getGroupCount(fullRenderHeight / downSize, 16), 1);

			m_ssaoTempFilter->transitionLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
		setPerfMarkerEnd(cmd);

		setPerfMarkerBegin(cmd, "SSAO-FilterY", { 0.9f, 0.5f, 0.9f, 1.0f });
		{
			m_renderer->getSceneTextures()->getSSAOMask()->transitionLayout(cmd, VK_IMAGE_LAYOUT_GENERAL);

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineFilterY);
			vkCmdDispatch(cmd, getGroupCount(fullRenderWidth / downSize, 16), getGroupCount(fullRenderHeight / downSize, 16), 1);

			m_renderer->getSceneTextures()->getSSAOMask()->transitionLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
		setPerfMarkerEnd(cmd);

		setPerfMarkerEnd(cmd);
	}

	void SSAO::createPipeline(bool bFirstInit)
	{
		m_ssaoTempFilter = RenderTexture::create(
			RHI::get()->getVulkanDevice(),
			m_renderer->getRenderWidth() / SceneTextures::SSAODownScaleSize(),
			m_renderer->getRenderHeight() / SceneTextures::SSAODownScaleSize(),
			SceneTextures::getSSAOMaskFormat()
		);

		m_ssaoTempFilter->transitionLayoutImmediately(RHI::get()->getGraphicsCommandPool(), RHI::get()->getGraphicsQueue(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		m_ssaoTemporal = RenderTexture::create(
			RHI::get()->getVulkanDevice(),
			m_renderer->getRenderWidth() / SceneTextures::SSAODownScaleSize(),
			m_renderer->getRenderHeight() / SceneTextures::SSAODownScaleSize(),
			SceneTextures::getSSAOMaskFormat()
		);

		m_ssaoTemporal->transitionLayoutImmediately(RHI::get()->getGraphicsCommandPool(), RHI::get()->getGraphicsQueue(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		// coomon set build.
		{
			VkDescriptorImageInfo inDepth = {};
			inDepth.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			inDepth.imageView = m_renderer->getSceneTextures()->getDepthStencil()->getDepthOnlyImageView();
			inDepth.sampler = RHI::get()->getLinearClampSampler();

			VkDescriptorImageInfo inGBufferS = {};
			inGBufferS.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			inGBufferS.imageView = m_renderer->getSceneTextures()->getGbufferS()->getImageView();
			inGBufferS.sampler = RHI::get()->getLinearClampSampler();

			VkDescriptorImageInfo inGBufferB = {};
			inGBufferB.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			inGBufferB.imageView = m_renderer->getSceneTextures()->getGbufferB()->getImageView();
			inGBufferB.sampler = RHI::get()->getLinearClampSampler();

			VkDescriptorImageInfo inGBufferA = {};
			inGBufferA.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			inGBufferA.imageView = m_renderer->getSceneTextures()->getGbufferA()->getImageView();
			inGBufferA.sampler = RHI::get()->getLinearClampSampler();

			VkDescriptorImageInfo SSAOMask = {};
			SSAOMask.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			SSAOMask.imageView = m_renderer->getSceneTextures()->getSSAOMask()->getImageView();

			VkDescriptorImageInfo inSSAOMask = {};
			inSSAOMask.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			inSSAOMask.imageView = m_renderer->getSceneTextures()->getSSAOMask()->getImageView();
			inSSAOMask.sampler = RHI::get()->getLinearClampSampler();

			VkDescriptorImageInfo SSAOTempFilter = {};
			SSAOTempFilter.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			SSAOTempFilter.imageView = m_ssaoTempFilter->getImageView();

			VkDescriptorImageInfo inSSAOTempFilter = {};
			inSSAOTempFilter.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			inSSAOTempFilter.imageView = m_ssaoTempFilter->getImageView();
			inSSAOTempFilter.sampler = RHI::get()->getLinearClampSampler();

			VkDescriptorImageInfo SSAOTemporal = {};
			SSAOTemporal.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			SSAOTemporal.imageView = m_ssaoTemporal->getImageView();

			VkDescriptorImageInfo inSSAOTemporal = {};
			inSSAOTemporal.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			inSSAOTemporal.imageView = m_ssaoTemporal->getImageView();
			inSSAOTemporal.sampler = RHI::get()->getLinearClampSampler();

			VkDescriptorImageInfo inVelocity = {};
			inVelocity.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			inVelocity.imageView = m_renderer->getSceneTextures()->getVelocity()->getImageView();
			inVelocity.sampler = RHI::get()->getLinearClampSampler();

			VkDescriptorImageInfo inHiz = {};
			inHiz.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			inHiz.imageView = m_renderer->getSceneTextures()->getHiz()->getImageView();
			inHiz.sampler = RHI::get()->getLinearClampSampler();

			RHI::get()->vkDescriptorFactoryBegin()
				.bindImage(0, &inDepth,    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
				.bindImage(1, &inGBufferS, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
				.bindImage(2, &inGBufferB, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
				.bindImage(3, &inGBufferA, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
				.bindImage(4, &SSAOMask,   VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          VK_SHADER_STAGE_COMPUTE_BIT)
				.bindImage(5, &inSSAOMask, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
				.bindImage(6, &SSAOTempFilter, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
				.bindImage(7, &inSSAOTempFilter, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
				.bindImage(8, &SSAOTemporal, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
				.bindImage(9, &inSSAOTemporal, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
				.bindImage(10, &inVelocity, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
				.bindImage(11, &inHiz, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
				.build(m_descriptorSets, m_descriptorSetLayouts);
		}

		// common pipeline layout.
		{
			VkPipelineLayoutCreateInfo plci = vkPipelineLayoutCreateInfo();
			plci.pushConstantRangeCount = 1;

			VkPushConstantRange push_constant{};
			push_constant.offset = 0;
			push_constant.size = sizeof(PushConstants);
			push_constant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			plci.pPushConstantRanges = &push_constant;

			std::vector<VkDescriptorSetLayout> setLayouts =
			{
				m_renderer->getFrameDataRing()->getLayout().layout,
				m_renderer->getViewDataRing()->getLayout().layout,
				m_descriptorSetLayouts.layout,
				m_renderer->getStaticTextures()->getGlobalBlueNoise()->setLayouts.layout
			};

			plci.setLayoutCount = (uint32_t)setLayouts.size();
			plci.pSetLayouts = setLayouts.data();

			m_pipelineLayout = RHI::get()->createPipelineLayout(plci);
		}

		// compute ao pipeline.
		{
			auto* shaderModule = RHI::get()->getShader("Shader/Spirv/SSAOComputeAO.comp.spv", true);

			VkPipelineShaderStageCreateInfo shaderStageCI{};
			shaderStageCI.module = shaderModule->GetModule();
			shaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStageCI.stage = VK_SHADER_STAGE_COMPUTE_BIT;
			shaderStageCI.pName = "main";

			VkComputePipelineCreateInfo computePipelineCreateInfo{};
			computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			computePipelineCreateInfo.layout = m_pipelineLayout;
			computePipelineCreateInfo.flags = 0;
			computePipelineCreateInfo.stage = shaderStageCI;
			vkCheck(vkCreateComputePipelines(RHI::get()->getDevice(), nullptr, 1, &computePipelineCreateInfo, nullptr, &m_pipelineComputeAO));
		}

		// filter X pipeline.
		{
			auto* shaderModule = RHI::get()->getShader("Shader/Spirv/SSAOSpatialFilterX.comp.spv", true);

			VkPipelineShaderStageCreateInfo shaderStageCI{};
			shaderStageCI.module = shaderModule->GetModule();
			shaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStageCI.stage = VK_SHADER_STAGE_COMPUTE_BIT;
			shaderStageCI.pName = "main";

			VkComputePipelineCreateInfo computePipelineCreateInfo{};
			computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			computePipelineCreateInfo.layout = m_pipelineLayout;
			computePipelineCreateInfo.flags = 0;
			computePipelineCreateInfo.stage = shaderStageCI;
			vkCheck(vkCreateComputePipelines(RHI::get()->getDevice(), nullptr, 1, &computePipelineCreateInfo, nullptr, &m_pipelineFilterX));
		}

		// filter Y pipeline.
		{
			auto* shaderModule = RHI::get()->getShader("Shader/Spirv/SSAOSpatialFilterY.comp.spv", true);

			VkPipelineShaderStageCreateInfo shaderStageCI{};
			shaderStageCI.module = shaderModule->GetModule();
			shaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStageCI.stage = VK_SHADER_STAGE_COMPUTE_BIT;
			shaderStageCI.pName = "main";

			VkComputePipelineCreateInfo computePipelineCreateInfo{};
			computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			computePipelineCreateInfo.layout = m_pipelineLayout;
			computePipelineCreateInfo.flags = 0;
			computePipelineCreateInfo.stage = shaderStageCI;
			vkCheck(vkCreateComputePipelines(RHI::get()->getDevice(), nullptr, 1, &computePipelineCreateInfo, nullptr, &m_pipelineFilterY));
		}

		// temporal filter
		{
			auto* shaderModule = RHI::get()->getShader("Shader/Spirv/SSAOTemporalFilter.comp.spv", true);

			VkPipelineShaderStageCreateInfo shaderStageCI{};
			shaderStageCI.module = shaderModule->GetModule();
			shaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStageCI.stage = VK_SHADER_STAGE_COMPUTE_BIT;
			shaderStageCI.pName = "main";

			VkComputePipelineCreateInfo computePipelineCreateInfo{};
			computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			computePipelineCreateInfo.layout = m_pipelineLayout;
			computePipelineCreateInfo.flags = 0;
			computePipelineCreateInfo.stage = shaderStageCI;
			vkCheck(vkCreateComputePipelines(RHI::get()->getDevice(), nullptr, 1, &computePipelineCreateInfo, nullptr, &m_pipelineTemporal));
		}

		// prev update.
		{
			auto* shaderModule = RHI::get()->getShader("Shader/Spirv/SSAOPrevInfoUpdate.comp.spv", true);

			VkPipelineShaderStageCreateInfo shaderStageCI{};
			shaderStageCI.module = shaderModule->GetModule();
			shaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStageCI.stage = VK_SHADER_STAGE_COMPUTE_BIT;
			shaderStageCI.pName = "main";

			VkComputePipelineCreateInfo computePipelineCreateInfo{};
			computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			computePipelineCreateInfo.layout = m_pipelineLayout;
			computePipelineCreateInfo.flags = 0;
			computePipelineCreateInfo.stage = shaderStageCI;
			vkCheck(vkCreateComputePipelines(RHI::get()->getDevice(), nullptr, 1, &computePipelineCreateInfo, nullptr, &m_pipelinePrevUpdate));
		}
	}

	void SSAO::destroyPipeline(bool bFinalRelease)
	{
		vkDestroyPipelineLayout(RHI::get()->getDevice(), m_pipelineLayout, nullptr);

		vkDestroyPipeline(RHI::get()->getDevice(), m_pipelineComputeAO, nullptr);
		vkDestroyPipeline(RHI::get()->getDevice(), m_pipelineFilterX, nullptr);
		vkDestroyPipeline(RHI::get()->getDevice(), m_pipelineFilterY, nullptr);

		vkDestroyPipeline(RHI::get()->getDevice(), m_pipelinePrevUpdate, nullptr);
		vkDestroyPipeline(RHI::get()->getDevice(), m_pipelineTemporal, nullptr);
		delete m_ssaoTempFilter;
		delete m_ssaoTemporal;
	}

}