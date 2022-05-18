#include "EpicAtmosphere.h"
#include "../../../Scene/Component/AtmosphereSky.h"
#include "../../../Engine.h"
#include "../../../Asset/AssetSystem.h"

namespace flower
{
	void EpicAtmospherePass::prepareSkyView(VkCommandBuffer cmd)
	{
		if (!m_renderer->getRenderScene()->ShouldRenderAtmosphere())
		{
			return;
		}
		auto atmosphereSky = m_renderer->getRenderScene()->getAtmosphereSky();
		AtmosphereParameters pushConstants = atmosphereSky->params;

		setPerfMarkerBegin(cmd, "Epic Atmosphere prepare Sky view Lut", { 0.5f, 0.5f, 0.3f, 1.0f });

		auto bindSet = [this, &cmd, &pushConstants]() 
		{
			std::vector<VkDescriptorSet> compPassSets = 
			{
				m_renderer->getFrameDataRing()->getSet().set,
				m_renderer->getViewDataRing()->getSet().set,
				m_descriptorSets.set,
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
				sizeof(AtmosphereParameters),
				&pushConstants
			);
		};

		bindSet();

		const uint32_t ThreadGroup = 8;

		setPerfMarkerBegin(cmd, "Compute-Transmittance", { 0.9f, 0.5f, 0.9f, 1.0f });
		{
			m_renderer->getStaticTextures()->getEpicAtmosphere()->transmittanceLut->transitionLayout(cmd, VK_IMAGE_LAYOUT_GENERAL);

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineTransmittance);
			vkCmdDispatch(cmd, 
				getGroupCount(EpicAtmosphere::TRANSMITTANCE_TEXTURE_WIDTH,  ThreadGroup), 
				getGroupCount(EpicAtmosphere::TRANSMITTANCE_TEXTURE_HEIGHT, ThreadGroup), 1);

			m_renderer->getStaticTextures()->getEpicAtmosphere()->transmittanceLut->transitionLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
		setPerfMarkerEnd(cmd);

		setPerfMarkerBegin(cmd, "Compute-MultiScatter", { 0.9f, 0.5f, 0.9f, 1.0f });
		{
			m_renderer->getStaticTextures()->getEpicAtmosphere()->multiScatterLut->transitionLayout(cmd, VK_IMAGE_LAYOUT_GENERAL);

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineMultiScatter);
			vkCmdDispatch(cmd,
				getGroupCount(EpicAtmosphere::MULTISCATTER_TEXTURE_WIDTH,  ThreadGroup),
				getGroupCount(EpicAtmosphere::MULTISCATTER_TEXTURE_HEIGHT, ThreadGroup), 1);

			m_renderer->getStaticTextures()->getEpicAtmosphere()->multiScatterLut->transitionLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
		setPerfMarkerEnd(cmd);

		setPerfMarkerBegin(cmd, "Compute-SkyView", { 0.9f, 0.5f, 0.9f, 1.0f });
		{
			m_renderer->getStaticTextures()->getEpicAtmosphere()->skyViewLut->transitionLayout(cmd, VK_IMAGE_LAYOUT_GENERAL);

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineSkyView);
			vkCmdDispatch(cmd,
				getGroupCount(EpicAtmosphere::SKYVIEW_TEXTURE_WIDTH, ThreadGroup),
				getGroupCount(EpicAtmosphere::SKYVIEW_TEXTURE_HEIGHT, ThreadGroup), 1);

			m_renderer->getStaticTextures()->getEpicAtmosphere()->skyViewLut->transitionLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
		setPerfMarkerEnd(cmd);

		setPerfMarkerEnd(cmd);
	}

	void EpicAtmospherePass::drawSky(VkCommandBuffer cmd)
	{
		if (!m_renderer->getRenderScene()->ShouldRenderAtmosphere())
		{
			return;
		}

		setPerfMarkerBegin(cmd, "Epic Atmosphere draw sky", { 0.5f, 0.5f, 0.3f, 1.0f });
		auto atmosphereSky = m_renderer->getRenderScene()->getAtmosphereSky();
		AtmosphereParameters pushConstants = atmosphereSky->params;
		auto bindSet = [this, &cmd, &pushConstants]()
		{
			std::vector<VkDescriptorSet> compPassSets =
			{
				m_renderer->getFrameDataRing()->getSet().set,
				m_renderer->getViewDataRing()->getSet().set,
				m_descriptorSets.set,
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
				sizeof(AtmosphereParameters),
				&pushConstants
			);
		};

		bindSet();

		const uint32_t ThreadGroup = 8;
		{
			m_renderer->getSceneTextures()->getHdrSceneColor()->transitionLayout(cmd, VK_IMAGE_LAYOUT_GENERAL);

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineDrawSky);
			vkCmdDispatch(cmd,
				getGroupCount(m_renderer->getSceneTextures()->getHdrSceneColor()->getExtent().width,  ThreadGroup),
				getGroupCount(m_renderer->getSceneTextures()->getHdrSceneColor()->getExtent().height, ThreadGroup), 1);

			m_renderer->getSceneTextures()->getHdrSceneColor()->transitionLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
		setPerfMarkerEnd(cmd);
	}

	void EpicAtmospherePass::createPipeline(bool bFirstInit)
	{
		if (m_textureManager == nullptr)
		{
			m_textureManager = GEngine.getRuntimeModule<AssetSystem>()->getTextureManager();
		}

		// coomon set build.
		{
			VkDescriptorImageInfo outTransmittanceLut = {};
			outTransmittanceLut.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			outTransmittanceLut.imageView = m_renderer->getStaticTextures()->getEpicAtmosphere()->transmittanceLut->getImageView();

			VkDescriptorImageInfo inTransmittanceLut = {};
			inTransmittanceLut.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			inTransmittanceLut.imageView = m_renderer->getStaticTextures()->getEpicAtmosphere()->transmittanceLut->getImageView();
			inTransmittanceLut.sampler = RHI::get()->getLinearClampSampler();

			VkDescriptorImageInfo outMultiScatterLut = {};
			outMultiScatterLut.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			outMultiScatterLut.imageView = m_renderer->getStaticTextures()->getEpicAtmosphere()->multiScatterLut->getImageView();

			VkDescriptorImageInfo inMultiScatterLut = {};
			inMultiScatterLut.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			inMultiScatterLut.imageView = m_renderer->getStaticTextures()->getEpicAtmosphere()->multiScatterLut->getImageView();
			inMultiScatterLut.sampler = RHI::get()->getLinearClampSampler();

			VkDescriptorImageInfo outSkyView = {};
			outSkyView.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			outSkyView.imageView = m_renderer->getStaticTextures()->getEpicAtmosphere()->skyViewLut->getImageView();

			VkDescriptorImageInfo inSkyView = {};
			inSkyView.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			inSkyView.imageView = m_renderer->getStaticTextures()->getEpicAtmosphere()->skyViewLut->getImageView();
			inSkyView.sampler = RHI::get()->getLinearClampSampler();

			VkDescriptorImageInfo inDepth = {};
			inDepth.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			inDepth.imageView = m_renderer->getSceneTextures()->getDepthStencil()->getDepthOnlyImageView();
			inDepth.sampler = RHI::get()->getLinearClampSampler();

			VkDescriptorImageInfo outHdrColor = {};
			outHdrColor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			outHdrColor.imageView = m_renderer->getSceneTextures()->getHdrSceneColor()->getImageView();

			RHI::get()->vkDescriptorFactoryBegin()
				.bindImage(0, &outTransmittanceLut, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          VK_SHADER_STAGE_COMPUTE_BIT)
				.bindImage(1, &inTransmittanceLut,  VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
				.bindImage(2, &outMultiScatterLut,  VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          VK_SHADER_STAGE_COMPUTE_BIT)
				.bindImage(3, &inMultiScatterLut,   VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
				.bindImage(4, &outSkyView,          VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          VK_SHADER_STAGE_COMPUTE_BIT)
				.bindImage(5, &inSkyView,           VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
				.bindImage(6, &inDepth,				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
				.bindImage(7, &outHdrColor,			VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,		   VK_SHADER_STAGE_COMPUTE_BIT)
				.build(m_descriptorSets, m_descriptorSetLayouts);
		}

		// common pipeline layout.
		{
			VkPipelineLayoutCreateInfo plci = vkPipelineLayoutCreateInfo();

			VkPushConstantRange pushRange = {};
			pushRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			pushRange.offset = 0;
			pushRange.size = sizeof(AtmosphereParameters);
			plci.pushConstantRangeCount = 1;
			plci.pPushConstantRanges = &pushRange;

			std::vector<VkDescriptorSetLayout> setLayouts =
			{
				m_renderer->getFrameDataRing()->getLayout().layout,
				m_renderer->getViewDataRing()->getLayout().layout,
				m_descriptorSetLayouts.layout,
			};

			plci.setLayoutCount = (uint32_t)setLayouts.size();
			plci.pSetLayouts = setLayouts.data();

			m_pipelineLayout = RHI::get()->createPipelineLayout(plci);
		}

		// compute transmittance pipeline.
		{
			auto* shaderModule = RHI::get()->getShader("Shader/Spirv/AtmosphereTransmittanceLut.comp.spv", true);

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
			vkCheck(vkCreateComputePipelines(RHI::get()->getDevice(), nullptr, 1, &computePipelineCreateInfo, nullptr, &m_pipelineTransmittance));
		}

		// compute multiScatter pipeline.
		{
			auto* shaderModule = RHI::get()->getShader("Shader/Spirv/AtmosphereMultipleScatteringLut.comp.spv", true);

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
			vkCheck(vkCreateComputePipelines(RHI::get()->getDevice(), nullptr, 1, &computePipelineCreateInfo, nullptr, &m_pipelineMultiScatter));
		}

		// compute sky view pipeline.
		{
			auto* shaderModule = RHI::get()->getShader("Shader/Spirv/AtmosphereSkyViewLut.comp.spv", true);

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
			vkCheck(vkCreateComputePipelines(RHI::get()->getDevice(), nullptr, 1, &computePipelineCreateInfo, nullptr, &m_pipelineSkyView));
		}

		// draw sky.
		{
			auto* shaderModule = RHI::get()->getShader("Shader/Spirv/AtmospherePostProcessSky.comp.spv", true);

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
			vkCheck(vkCreateComputePipelines(RHI::get()->getDevice(), nullptr, 1, &computePipelineCreateInfo, nullptr, &m_pipelineDrawSky));
		}
	}

	void EpicAtmospherePass::destroyPipeline(bool bFinalRelease)
	{
		vkDestroyPipelineLayout(RHI::get()->getDevice(), m_pipelineLayout, nullptr);

		vkDestroyPipeline(RHI::get()->getDevice(), m_pipelineTransmittance, nullptr);
		vkDestroyPipeline(RHI::get()->getDevice(), m_pipelineMultiScatter, nullptr);
		vkDestroyPipeline(RHI::get()->getDevice(), m_pipelineSkyView, nullptr);
		vkDestroyPipeline(RHI::get()->getDevice(), m_pipelineDrawSky, nullptr);
	}
}