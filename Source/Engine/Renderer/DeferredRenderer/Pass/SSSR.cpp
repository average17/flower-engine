#include "SSSR.h"
#include "../../../Asset/AssetSystem.h"
#include "../../../Engine.h"
#include "../BlueNoise.h"

namespace flower
{
	static AutoCVarFloat cVarSSRMaxRoughness(
		"r.SSR.MaxRoughness",
		"ssr max trace roughness.",
		"SSR",
		0.7f,
		CVarFlags::ReadAndWrite
	);

	static AutoCVarFloat cVarSSRThickness(
		"r.SSR.thickness",
		"ssr max trace thickness.",
		"SSR",
		0.1f,
		CVarFlags::ReadAndWrite
	);

	static AutoCVarFloat cVarSSRTemporalWeight(
		"r.SSR.temporalWeight",
		"ssr temporal weight.",
		"SSR",
		0.6f,
		CVarFlags::ReadAndWrite
	);

	static AutoCVarFloat cVarSSRTemporalClampScale(
		"r.SSR.temporalClampScale",
		"ssr temporal clamp scale.",
		"SSR",
		1.25f,
		CVarFlags::ReadAndWrite
	);

	struct PushConstants
	{
		glm::uvec2 dimXY; // .xy is full res width and height
		float blueNoiseDimX;
		float blueNoiseDimY;
		float thickness;
		float cut;
		float maxRoughness;
		float temporalWeight;
		float temporalClampScale;
	};

	void SSSR::dynamicRender(VkCommandBuffer cmd)
	{
		if (m_renderer->getRenderTimes() < 2) { return; }

		setPerfMarkerBegin(cmd, "SSSR", { 0.2f, 0.5f, 0.3f, 1.0f });

		PushConstants pushConst{};
		glm::vec2 blueNoiseOffset = getHalton23RandomOffset(m_renderer->getRenderTimes());
		pushConst.blueNoiseDimX = blueNoiseOffset.x;
		pushConst.blueNoiseDimY = blueNoiseOffset.y;
		pushConst.thickness = cVarSSRThickness.get();

		const auto renderTimes = m_renderer->getRenderTimes();
		constexpr uint64_t lerpTimes = 120u;

		// remap [0,1]
		auto lerpFactor = float(glm::min(renderTimes, lerpTimes)) / float(lerpTimes);
		lerpFactor = glm::smoothstep(0.0f, 1.0f, lerpFactor);

		pushConst.temporalClampScale = cVarSSRTemporalClampScale.get() * glm::max(0.5f, lerpFactor);
		pushConst.temporalWeight = cVarSSRTemporalWeight.get() * lerpFactor;
		pushConst.maxRoughness = cVarSSRMaxRoughness.get();
		pushConst.dimXY = { m_renderer->getRenderWidth(), m_renderer->getRenderHeight() };
		pushConst.cut = renderTimes != 0 ? 1.0f : 0.0f;


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
		setPerfMarkerBegin(cmd, "SSSR-Intersect", { 0.2f, 0.5f, 0.9f, 1.0f });
		{
			
			m_renderer->getSceneTextures()->getSSRHit()->transitionLayout(cmd, VK_IMAGE_LAYOUT_GENERAL);
			m_renderer->getSceneTextures()->getSSRIntersect()->transitionLayout(cmd, VK_IMAGE_LAYOUT_GENERAL);

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineIntersect);
			vkCmdDispatch(cmd, getGroupCount(pushConst.dimXY.x, 16), getGroupCount(pushConst.dimXY.y, 16), 1);

			m_renderer->getSceneTextures()->getSSRHit()->transitionLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			m_renderer->getSceneTextures()->getSSRIntersect()->transitionLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
		setPerfMarkerEnd(cmd);

		setPerfMarkerBegin(cmd, "SSSR-Resolve", { 0.2f, 0.3f, 0.9f, 1.0f });
		{

			m_renderer->getSceneTextures()->getSSRReflection()->transitionLayout(cmd, VK_IMAGE_LAYOUT_GENERAL);

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineResolve);
			vkCmdDispatch(cmd, getGroupCount(pushConst.dimXY.x, 16), getGroupCount(pushConst.dimXY.y, 16), 1);

			m_renderer->getSceneTextures()->getSSRReflection()->transitionLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
		setPerfMarkerEnd(cmd);

		setPerfMarkerBegin(cmd, "SSSR-Temporal", { 0.6f, 0.4f, 0.3f, 1.0f });
		{
			m_renderer->getSceneTextures()->getSSRTemporalFilter()->transitionLayout(cmd, VK_IMAGE_LAYOUT_GENERAL);

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineTemporal);
			vkCmdDispatch(cmd, getGroupCount(pushConst.dimXY.x, 16), getGroupCount(pushConst.dimXY.y, 16), 1);

			m_renderer->getSceneTextures()->getSSRTemporalFilter()->transitionLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
		setPerfMarkerEnd(cmd);

		setPerfMarkerBegin(cmd, "SSSR-Update", { 0.6f, 0.4f, 0.5f, 1.0f });
		{
			m_renderer->getSceneTextures()->getSSRReflectionPrev()->transitionLayout(cmd, VK_IMAGE_LAYOUT_GENERAL);

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineUpdate);
			vkCmdDispatch(cmd, getGroupCount(pushConst.dimXY.x, 16), getGroupCount(pushConst.dimXY.y, 16), 1);
			m_renderer->getSceneTextures()->getSSRReflectionPrev()->transitionLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
		setPerfMarkerEnd(cmd);
		
		setPerfMarkerEnd(cmd);
	}

	void SSSR::createPipeline(bool bFirstInit)
	{	
		if (m_textureManager == nullptr)
		{
			m_textureManager = GEngine.getRuntimeModule<AssetSystem>()->getTextureManager();
		}

		// coomon set build.
		{
			VkDescriptorImageInfo inHistory = {};
			inHistory.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			inHistory.imageView = m_renderer->getSceneTextures()->getHistory()->getImageView();
			inHistory.sampler = RHI::get()->getLinearClampNoMipSampler();

			VkDescriptorImageInfo inGBufferS = {};
			inGBufferS.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			inGBufferS.imageView = m_renderer->getSceneTextures()->getGbufferS()->getImageView();
			inGBufferS.sampler = RHI::get()->getLinearClampNoMipSampler();

			VkDescriptorImageInfo inGBufferB = {};
			inGBufferB.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			inGBufferB.imageView = m_renderer->getSceneTextures()->getGbufferB()->getImageView();
			inGBufferB.sampler = RHI::get()->getLinearClampNoMipSampler();

			VkDescriptorImageInfo inHiz = {};
			inHiz.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			inHiz.imageView = m_renderer->getSceneTextures()->getHiz()->getImageView();
			inHiz.sampler = RHI::get()->getPointClampEdgeSampler();
			 
			VkDescriptorImageInfo inBlueNoise1Spp = {};
			inBlueNoise1Spp.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			inBlueNoise1Spp.imageView = m_renderer->getStaticTextures()->getBlueNoise1Spp()->getImageView();
			inBlueNoise1Spp.sampler = RHI::get()->getPointClampEdgeSampler();

			VkDescriptorImageInfo outSSRReflection = {};
			outSSRReflection.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			outSSRReflection.imageView = m_renderer->getSceneTextures()->getSSRReflection()->getImageView();

			VkDescriptorImageInfo outSSRReflectionPrev = {};
			outSSRReflectionPrev.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			outSSRReflectionPrev.imageView = m_renderer->getSceneTextures()->getSSRReflectionPrev()->getImageView();

			VkDescriptorImageInfo inSSRReflection = {};
			inSSRReflection.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			inSSRReflection.imageView = m_renderer->getSceneTextures()->getSSRReflection()->getImageView();
			inSSRReflection.sampler = RHI::get()->getLinearClampSampler();

			VkDescriptorImageInfo inSSRReflectionPrev = {};
			inSSRReflectionPrev.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			inSSRReflectionPrev.imageView = m_renderer->getSceneTextures()->getSSRReflectionPrev()->getImageView();
			inSSRReflectionPrev.sampler = RHI::get()->getLinearClampSampler();

			VkDescriptorImageInfo inVelocity = {};
			inVelocity.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			inVelocity.imageView = m_renderer->getSceneTextures()->getVelocity()->getImageView();
			inVelocity.sampler = RHI::get()->getLinearClampSampler();

			VkDescriptorImageInfo inTemporalFilter = {};
			inTemporalFilter.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			inTemporalFilter.imageView = m_renderer->getSceneTextures()->getSSRTemporalFilter()->getImageView();
			inTemporalFilter.sampler = RHI::get()->getLinearClampSampler();

			VkDescriptorImageInfo outTemporalFilter = {};
			outTemporalFilter.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			outTemporalFilter.imageView = m_renderer->getSceneTextures()->getSSRTemporalFilter()->getImageView();

			VkDescriptorImageInfo inGBufferA = {};
			inGBufferA.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			inGBufferA.imageView = m_renderer->getSceneTextures()->getGbufferA()->getImageView();
			inGBufferA.sampler = RHI::get()->getLinearClampSampler();

			VkDescriptorImageInfo SSRIntersect = {};
			SSRIntersect.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			SSRIntersect.imageView = m_renderer->getSceneTextures()->getSSRIntersect()->getImageView();

			VkDescriptorImageInfo SSRHitMask = {};
			SSRHitMask.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			SSRHitMask.imageView = m_renderer->getSceneTextures()->getSSRHit()->getImageView();

			// intersect use point clamp.
			// we store pdf on .a, use linear sampler may interpolate error e
			VkDescriptorImageInfo inSSRIntersect = {};
			inSSRIntersect.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			inSSRIntersect.imageView = m_renderer->getSceneTextures()->getSSRIntersect()->getImageView();
			inSSRIntersect.sampler = RHI::get()->getLinearClampSampler();

			VkDescriptorImageInfo inSSRHitMask = {};
			inSSRHitMask.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			inSSRHitMask.imageView = m_renderer->getSceneTextures()->getSSRHit()->getImageView();
			inSSRHitMask.sampler = RHI::get()->getLinearClampSampler();

			VkDescriptorImageInfo inDepth = {};
			inDepth.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			inDepth.imageView = m_renderer->getSceneTextures()->getDepthStencil()->getDepthOnlyImageView();
			inDepth.sampler = RHI::get()->getLinearClampSampler();

			RHI::get()->vkDescriptorFactoryBegin()
				.bindImage(0, &inHistory,  VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
				.bindImage(1, &inGBufferS,  VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
				.bindImage(2, &inGBufferB,  VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
				.bindImage(3, &inHiz,       VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
				.bindImage(4, &inBlueNoise1Spp, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
				.bindImage(5, &outSSRReflection, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
				.bindImage(6, &outSSRReflectionPrev, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
				.bindImage(7, &inSSRReflection, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
				.bindImage(8, &inSSRReflectionPrev, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
				.bindImage(9, &inVelocity, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
				.bindImage(10, &inTemporalFilter, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
				.bindImage(11, &outTemporalFilter, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
				.bindImage(12, &inGBufferA, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
				.bindImage(13, &SSRIntersect, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
				.bindImage(14, &SSRHitMask, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
				.bindImage(15, &inSSRIntersect, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
				.bindImage(16, &inSSRHitMask, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
				.bindImage(17, &inDepth, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
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

		// intersect pipeline.
		{
			auto* shaderModule = RHI::get()->getShader("Shader/Spirv/SSRIntersect.comp.spv", true);

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
			vkCheck(vkCreateComputePipelines(RHI::get()->getDevice(), nullptr, 1, &computePipelineCreateInfo, nullptr, &m_pipelineIntersect));
		}

		// resolve
		{
			auto* shaderModule = RHI::get()->getShader("Shader/Spirv/SSRResolve.comp.spv", true);

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
			vkCheck(vkCreateComputePipelines(RHI::get()->getDevice(), nullptr, 1, &computePipelineCreateInfo, nullptr, &m_pipelineResolve));
		}

		// temporal pipeline.
		{
			auto* shaderModule = RHI::get()->getShader("Shader/Spirv/SSRTemporal.comp.spv", true);

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

		// ssr update.
		{
			auto* shaderModule = RHI::get()->getShader("Shader/Spirv/SSRPrevInfoUpdate.comp.spv", true);

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
			vkCheck(vkCreateComputePipelines(RHI::get()->getDevice(), nullptr, 1, &computePipelineCreateInfo, nullptr, &m_pipelineUpdate));
		}
	}

	void SSSR::destroyPipeline(bool bFinalRelease)
	{
		vkDestroyPipelineLayout(RHI::get()->getDevice(), m_pipelineLayout, nullptr);
		vkDestroyPipeline(RHI::get()->getDevice(), m_pipelineIntersect, nullptr);
		vkDestroyPipeline(RHI::get()->getDevice(), m_pipelineResolve, nullptr);
		vkDestroyPipeline(RHI::get()->getDevice(), m_pipelineTemporal, nullptr);
		vkDestroyPipeline(RHI::get()->getDevice(), m_pipelineUpdate, nullptr);
	}

}