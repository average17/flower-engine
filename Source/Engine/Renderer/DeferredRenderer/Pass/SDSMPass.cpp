#include "SDSMPass.h"
#include "../SceneTextures.h"
#include "../DefferedRenderer.h"
#include "../RenderScene.h"
#include "../SceneStaticUniform.h"
#include "../../../Engine.h"
#include "../../../Asset/AssetSystem.h"

namespace flower
{
	SDSMRenderResource GSDSMRenderResource;

	static AutoCVarInt32 cVarShadowDepthDimXY(
		"r.Shadow.DimXY",
		"Per page dim xy.",
		"Shadow",
		2048,
		CVarFlags::ReadAndWrite | CVarFlags::RenderStateRelative
	);

	static AutoCVarInt32 cVarCascadeCount(
		"r.Shadow.CascadeCount",
		"cascade shadow count.",
		"Shadow",
		4,
		CVarFlags::ReadAndWrite | CVarFlags::RenderStateRelative
	);

	static AutoCVarInt32 cVarShadowEnableDepthClamp(
		"r.Shadow.DepthClamp",
		"Enable depth clamp on shadow.",
		"Shadow",
		1,
		CVarFlags::ReadAndWrite
	);

	static AutoCVarInt32 cVarShadowFixCascade(
		"r.Shadow.FixCascade",
		"Use fix distance cascade.",
		"Shadow",
		0,
		CVarFlags::ReadAndWrite
	);

	static AutoCVarInt32 cVarMaxShadowDistance(
		"r.Shadow.Distance",
		"Shadow draw distance.",
		"Shadow",
		300,
		CVarFlags::ReadAndWrite
	);

	static AutoCVarFloat cVarCascadeLambda(
		"r.Shadow.CascadeLambda",
		"Shadow cascade split lambda.",
		"Shadow",
		0.95f,
		CVarFlags::ReadAndWrite
	);

	static AutoCVarFloat cVarShadowBiasConstant(
		"r.Shadow.BiasConstant",
		"Shadow bias constant.",
		"Shadow",
		1.25f,
		CVarFlags::ReadAndWrite
	);

	static AutoCVarFloat cVarShadowBiasSlope(
		"r.Shadow.BiasSlope",
		"Shadow bias slope.",
		"Shadow",
		1.75f,
		CVarFlags::ReadAndWrite
	);

	uint32_t getShadowPerPageDimXY()
	{
		return getNextPOT(uint32_t(cVarShadowDepthDimXY.get()));
	}

	uint32_t getShadowCascadeCount()
	{
		return uint32_t(glm::clamp(cVarCascadeCount.get(), 2, MAX_CACSADE_COUNT));
	}

	struct CascadeSetupPushConstant
	{
		uint32_t bFixCascade;
		uint32_t hizLastMipLevels;
		uint32_t cascadeCount;
		uint32_t perCascadeXYDim;

		float farShadowPos;
		float splitLambda;
	};

	struct CascadeCullingPushConst
	{
		uint32_t count;
		uint32_t cascadeCount;
	};

	struct CascadeDepthPushConstantsData
	{
		uint32_t cascadeId;
	};

	void SDSMPass::onSceneTextureRecreate(uint32_t width, uint32_t height)
	{
		releaseInner();
		initInner();
	}

	void SDSMPass::dynamicRecord(VkCommandBuffer cmd)
	{
		const uint32_t newCount = getShadowCascadeCount();
		const uint32_t newDimXY = getShadowPerPageDimXY();

		setPerfMarkerBegin(cmd, "SDSM InstanceMerge", { 0.1f, 1.0f, 0.2f, 1.0f });

		// pass #0. min max evaluate. 
		setPerfMarkerBegin(cmd, "SDSM MinMax evaluate", { 0.1f, 1.0f, 0.3f, 1.0f });
		{
			std::array<VkImageMemoryBarrier, 1> imageBarriers{};
			imageBarriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageBarriers[0].image = m_renderer->getSceneTextures()->getDepthStencil()->getImage();
			imageBarriers[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			imageBarriers[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			imageBarriers[0].subresourceRange.levelCount = 1;
			imageBarriers[0].subresourceRange.layerCount = 1;
			imageBarriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			imageBarriers[0].oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageBarriers[0].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			vkCmdPipelineBarrier(cmd,
				VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				0, 0, nullptr, 0, nullptr, (uint32_t)imageBarriers.size(), imageBarriers.data());

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_depthMinMaxEvaluate.pipeline);

			std::vector<VkDescriptorSet> compPassSets = {
				  m_depthMinMaxEvaluate.set.set
				, GSDSMRenderResource.depthRangeBuffer.set.set
			};

			vkCmdBindDescriptorSets(
				cmd,
				VK_PIPELINE_BIND_POINT_COMPUTE,
				m_depthMinMaxEvaluate.pipelineLayout,
				0,
				(uint32_t)compPassSets.size(),
				compPassSets.data(),
				0,
				nullptr
			);

			vkCmdDispatch(cmd,
				getGroupCount(m_renderer->getSceneTextures()->getDepthStencil()->getExtent().width, 8),
				getGroupCount(m_renderer->getSceneTextures()->getDepthStencil()->getExtent().height, 8),
				1
			);
		}
		setPerfMarkerEnd(cmd);

		// pass #1. cascade setup.
		setPerfMarkerBegin(cmd, "SDSM cascade setup", { 0.1f, 1.0f, 0.4f, 1.0f });
		{
			VkBufferMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			barrier.buffer = *GSDSMRenderResource.depthRangeBuffer.ssbo;
			barrier.size = GSDSMRenderResource.depthRangeBuffer.size;
			barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			vkCmdPipelineBarrier(cmd,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				0, 0, nullptr, 1, &barrier, 0, nullptr);

			CascadeSetupPushConstant pushConstants{};
			pushConstants.cascadeCount = getShadowCascadeCount();
			pushConstants.perCascadeXYDim = getShadowPerPageDimXY();
			pushConstants.splitLambda = cVarCascadeLambda.get();
			pushConstants.hizLastMipLevels = m_renderer->getSceneTextures()->getHiz()->getInfo().mipLevels - 1; // last miplevel.
			pushConstants.farShadowPos = (float)cVarMaxShadowDistance.get();
			pushConstants.bFixCascade = (uint32_t)cVarShadowFixCascade.get();

			vkCmdPushConstants(cmd, m_cascadeSetup.pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(CascadeSetupPushConstant), &pushConstants);

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_cascadeSetup.pipeline);

			std::vector<VkDescriptorSet> compPassSets = {
				m_renderer->getFrameDataRing()->getSet().set,
				m_renderer->getViewDataRing()->getSet().set,
				GSDSMRenderResource.depthRangeBuffer.set.set,
				GSDSMRenderResource.cascadeInfoBuffer.set.set,
				m_cascadeSetup.set.set
			};

			vkCmdBindDescriptorSets(
				cmd,
				VK_PIPELINE_BIND_POINT_COMPUTE,
				m_cascadeSetup.pipelineLayout,
				0,
				(uint32_t)compPassSets.size(),
				compPassSets.data(),
				0,
				nullptr
			);

			vkCmdDispatch(cmd, 1, 1, 1);
		}
		setPerfMarkerEnd(cmd);

		// pass #2. draw call build.
		setPerfMarkerBegin(cmd, "SDSM drawcall build", { 0.1f, 1.0f, 0.5f, 1.0f });
		{
			VkBufferMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			barrier.buffer = *GSDSMRenderResource.cascadeInfoBuffer.ssbo;
			barrier.size = GSDSMRenderResource.cascadeInfoBuffer.size;
			barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			vkCmdPipelineBarrier(cmd,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				0, 0, nullptr, 1, &barrier, 0, nullptr);

			CascadeCullingPushConst pushConstants{};
			pushConstants.cascadeCount = getShadowCascadeCount();
			pushConstants.count = (uint32_t)m_renderer->getRenderScene()->getMeshObjectSSBOData().size();

			vkCmdPushConstants(cmd, m_cascadeCulling.pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(CascadeCullingPushConst), &pushConstants);

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_cascadeCulling.pipeline);

			std::vector<VkDescriptorSet> compPassSets = {
					m_renderer->getFrameDataRing()->getSet().set,
					m_renderer->getViewDataRing()->getSet().set,
					m_renderer->getObjectDataRing()->getSet().set,
					GSDSMRenderResource.indirectCasccadeBuffer.descriptorSets.set,
					GSDSMRenderResource.indirectCasccadeBuffer.countDescriptorSets.set,
					GSDSMRenderResource.cascadeInfoBuffer.set.set
			};

			vkCmdBindDescriptorSets(
				cmd,
				VK_PIPELINE_BIND_POINT_COMPUTE,
				m_cascadeCulling.pipelineLayout,
				0,
				(uint32_t)compPassSets.size(),
				compPassSets.data(),
				0,
				nullptr
			);

			vkCmdDispatch(cmd, (pushConstants.count * pushConstants.cascadeCount) / 256 + 1, 1, 1);
		}
		setPerfMarkerEnd(cmd);

		// pass #3. draw texture.
		uint32_t countObject = (uint32_t)m_renderer->getRenderScene()->getMeshObjectSSBOData().size();
		if (countObject > 0)
		{
			setPerfMarkerBegin(cmd, "SDSM Depth draw", { 0.1f, 1.0f, 0.6f, 1.0f });

			VkExtent2D sceneTextureExtent2D{};
			sceneTextureExtent2D.width  = m_renderer->getSceneTextures()->getShadowDepth()->getExtent().width;
			sceneTextureExtent2D.height = m_renderer->getSceneTextures()->getShadowDepth()->getExtent().height;

			VkRenderPassBeginInfo rpInfo = vkRenderpassBeginInfo(
				m_cascadeDepth.renderpass, sceneTextureExtent2D, m_cascadeDepth.framebuffer);

			VkClearValue clearVals{};
			clearVals.depthStencil = { 0.0f, 1 };
			rpInfo.clearValueCount = 1;
			rpInfo.pClearValues = &clearVals;

			m_meshManager->bindIndexBuffer(cmd);
			m_meshManager->bindVertexBuffer(cmd);

			std::array<VkBufferMemoryBarrier, 2> barriers{};
			barriers[0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			barriers[0].buffer = GSDSMRenderResource.indirectCasccadeBuffer.drawIndirectSSBO->getVkBuffer();
			barriers[0].size = VK_WHOLE_SIZE;
			barriers[0].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			barriers[0].dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;

			barriers[1].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			barriers[1].buffer = GSDSMRenderResource.indirectCasccadeBuffer.countBuffer->getVkBuffer();
			barriers[1].size = GSDSMRenderResource.indirectCasccadeBuffer.countSize;
			barriers[1].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			barriers[1].dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;

			vkCmdPipelineBarrier(
				cmd,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
				0,
				0, nullptr,
				(uint32_t)barriers.size(),
				barriers.data(),
				0,
				nullptr
			);

			vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
			{
				std::vector<VkDescriptorSet> meshPassSets = {
						m_textureManager->getBindless()->getSet(),                  // 0
						RHI::get()->getBindlessSampler()->getSet(),                 // 1
						m_renderer->getViewDataRing()->getSet().set,			    // 2
						m_renderer->getFrameDataRing()->getSet().set,			    // 3
						m_renderer->getObjectDataRing()->getSet().set,			    // 4
						m_renderer->getMaterialDataRing()->getSet().set,			// 5
						GSDSMRenderResource.indirectCasccadeBuffer.descriptorSets.set, // 6
						GSDSMRenderResource.cascadeInfoBuffer.set.set			    // 7
				};

				vkCmdBindDescriptorSets(
					cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
					m_cascadeDepth.pipelineLayout,
					0,
					(uint32_t)meshPassSets.size(),
					meshPassSets.data(),
					0,
					nullptr
				);
				vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_cascadeDepth.pipeline);

				for (uint32_t id = 0; id < getShadowCascadeCount(); id++)
				{
					const auto dimXY = getShadowPerPageDimXY();

					VkViewport viewport{};
					viewport.minDepth = 0.0f;
					viewport.maxDepth = 1.0f;
					viewport.y = (float)dimXY;
					viewport.height = -(float)dimXY;


					viewport.x = (float)dimXY * (float)id;
					viewport.width = (float)dimXY;

					vkCmdSetViewport(cmd, 0, 1, &viewport);

					VkRect2D scissor{};
					scissor.extent = { dimXY, dimXY};
					scissor.offset = { int32_t(dimXY * id), 0 };
					vkCmdSetScissor(cmd, 0, 1, &scissor);

					vkCmdSetDepthBias(cmd, -1.0f * cVarShadowBiasConstant.get(), 0, -1.0f * cVarShadowBiasSlope.get());

					CascadeDepthPushConstantsData pushConstants{};
					pushConstants.cascadeId = id;

					vkCmdPushConstants(cmd, m_cascadeDepth.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(CascadeDepthPushConstantsData), &pushConstants);

					vkCmdDrawIndexedIndirectCount(
						cmd,
						GSDSMRenderResource.indirectCasccadeBuffer.drawIndirectSSBO->getVkBuffer(),
						VkDeviceSize(id * MAX_SSBO_OBJECTS * sizeof(GPUDrawCallData)),
						GSDSMRenderResource.indirectCasccadeBuffer.countBuffer->getVkBuffer(),
						id * sizeof(GPUOutIndirectDrawCount),
						countObject,
						sizeof(GPUDrawCallData)
					);
				}
			}
			vkCmdEndRenderPass(cmd);
			setPerfMarkerEnd(cmd);
		}

		setPerfMarkerEnd(cmd);
	}

	void SDSMPass::initInner()
	{
		m_meshManager = GEngine.getRuntimeModule<AssetSystem>()->getMeshManager();
		m_textureManager = GEngine.getRuntimeModule<AssetSystem>()->getTextureManager();

		GSDSMRenderResource.init();

		m_depthMinMaxEvaluate.init(this);
		m_cascadeSetup.init(this);
		m_cascadeCulling.init(this);

		m_cascadeDepth.init(this);
	}
	
	void SDSMPass::releaseInner()
	{
		m_depthMinMaxEvaluate.release();
		m_cascadeSetup.release();
		m_cascadeCulling.release();
		m_cascadeDepth.release();

		GSDSMRenderResource.release();
	}

	void SDSMPass::DepthMinMaxEvaluate::init(SDSMPass* in)
	{
		CHECK(pipeline == VK_NULL_HANDLE && pipelineLayout == VK_NULL_HANDLE);

		// build descriptor set and set layout.
		{
			VkDescriptorImageInfo depthImage = {};
			depthImage.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			depthImage.imageView = in->m_renderer->getSceneTextures()->getDepthStencil()->getDepthOnlyImageView();
			depthImage.sampler = RHI::get()->getPointClampEdgeSampler();

			RHI::get()->vkDescriptorFactoryBegin()
				.bindImage(0, &depthImage, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
				.build(set, setLayout);
		}

		VkPipelineLayoutCreateInfo plci = vkPipelineLayoutCreateInfo();
		std::vector<VkDescriptorSetLayout> setLayouts =
		{
			setLayout.layout,
			GSDSMRenderResource.depthRangeBuffer.setLayout.layout,
		};

		plci.setLayoutCount = (uint32_t)setLayouts.size();
		plci.pSetLayouts = setLayouts.data();
		pipelineLayout = RHI::get()->createPipelineLayout(plci);

		auto* shaderModule = RHI::get()->getShader("Shader/Spirv/SDSMDepthMinMax.comp.spv", true);

		VkPipelineShaderStageCreateInfo shaderStageCI{};
		shaderStageCI.module = shaderModule->GetModule();
		shaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageCI.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		shaderStageCI.pName = "main";

		VkComputePipelineCreateInfo computePipelineCreateInfo{};
		computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		computePipelineCreateInfo.layout = pipelineLayout;
		computePipelineCreateInfo.flags = 0;
		computePipelineCreateInfo.stage = shaderStageCI;
		vkCheck(vkCreateComputePipelines(RHI::get()->getDevice(), nullptr, 1, &computePipelineCreateInfo, nullptr, &pipeline));
	}

	void SDSMPass::DepthMinMaxEvaluate::release()
	{
		CHECK( pipeline != VK_NULL_HANDLE && pipelineLayout != VK_NULL_HANDLE);
		vkDestroyPipeline(RHI::get()->getDevice(), pipeline, nullptr);
		vkDestroyPipelineLayout(RHI::get()->getDevice(), pipelineLayout, nullptr);
		pipeline = VK_NULL_HANDLE;
		pipelineLayout = VK_NULL_HANDLE;
	}

	void SDSMPass::CascadeSetup::init(SDSMPass* in)
	{
		CHECK(pipeline == VK_NULL_HANDLE && pipelineLayout == VK_NULL_HANDLE);

		// build descriptor set and set layout.
		{
			VkDescriptorImageInfo hiZImage = {};
			hiZImage.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			hiZImage.imageView = in->m_renderer->getSceneTextures()->getHiz()->getImageView();
			hiZImage.sampler = RHI::get()->getPointClampEdgeSampler();

			RHI::get()->vkDescriptorFactoryBegin()
				.bindImage(0, &hiZImage, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
				.build(set, setLayout);
		}

		VkPipelineLayoutCreateInfo plci = vkPipelineLayoutCreateInfo();
		std::vector<VkDescriptorSetLayout> setLayouts =
		{
			in->m_renderer->getFrameDataRing()->getLayout().layout,
			in->m_renderer->getViewDataRing()->getLayout().layout,
			GSDSMRenderResource.depthRangeBuffer.setLayout.layout,
			GSDSMRenderResource.cascadeInfoBuffer.setLayout.layout,
			setLayout.layout
		};


		VkPushConstantRange pushConstantsRange{};
		pushConstantsRange.offset = 0;
		pushConstantsRange.size = sizeof(CascadeSetupPushConstant);
		pushConstantsRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		plci.pushConstantRangeCount = 1;
		plci.pPushConstantRanges = &pushConstantsRange;

		plci.setLayoutCount = (uint32_t)setLayouts.size();
		plci.pSetLayouts = setLayouts.data();
		pipelineLayout = RHI::get()->createPipelineLayout(plci);

		auto* shaderModule = RHI::get()->getShader("Shader/Spirv/SDSMShadowSetup.comp.spv", true);

		VkPipelineShaderStageCreateInfo shaderStageCI{};
		shaderStageCI.module = shaderModule->GetModule();
		shaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageCI.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		shaderStageCI.pName = "main";

		VkComputePipelineCreateInfo computePipelineCreateInfo{};
		computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		computePipelineCreateInfo.layout = pipelineLayout;
		computePipelineCreateInfo.flags = 0;
		computePipelineCreateInfo.stage = shaderStageCI;
		vkCheck(vkCreateComputePipelines(RHI::get()->getDevice(), nullptr, 1, &computePipelineCreateInfo, nullptr, &pipeline));
	}

	void SDSMPass::CascadeSetup::release()
	{
		CHECK(pipeline != VK_NULL_HANDLE && pipelineLayout != VK_NULL_HANDLE);
		vkDestroyPipeline(RHI::get()->getDevice(), pipeline, nullptr);
		vkDestroyPipelineLayout(RHI::get()->getDevice(), pipelineLayout, nullptr);
		pipeline = VK_NULL_HANDLE;
		pipelineLayout = VK_NULL_HANDLE;
	}

	void SDSMPass::CascadeCulling::init(SDSMPass* in)
	{
		CHECK(pipeline == VK_NULL_HANDLE && pipelineLayout == VK_NULL_HANDLE);

		VkPipelineLayoutCreateInfo plci = vkPipelineLayoutCreateInfo();

		std::vector<VkDescriptorSetLayout> setLayouts =
		{
			in->m_renderer->getFrameDataRing()->getLayout().layout,
			in->m_renderer->getViewDataRing()->getLayout().layout,
			in->m_renderer->getObjectDataRing()->getLayout().layout,
			GSDSMRenderResource.indirectCasccadeBuffer.descriptorSetLayout.layout,
			GSDSMRenderResource.indirectCasccadeBuffer.countDescriptorSetLayout.layout,
			GSDSMRenderResource.cascadeInfoBuffer.setLayout.layout
		};

		VkPushConstantRange pushConstantsRange{};
		pushConstantsRange.offset = 0;
		pushConstantsRange.size = sizeof(CascadeCullingPushConst);
		pushConstantsRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		plci.pushConstantRangeCount = 1;
		plci.pPushConstantRanges = &pushConstantsRange;

		plci.setLayoutCount = (uint32_t)setLayouts.size();
		plci.pSetLayouts = setLayouts.data();
		pipelineLayout = RHI::get()->createPipelineLayout(plci);

		auto* shaderModule = RHI::get()->getShader("Shader/Spirv/SDSMBuildDrawCall.comp.spv", true);

		VkPipelineShaderStageCreateInfo shaderStageCI{};
		shaderStageCI.module = shaderModule->GetModule();
		shaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageCI.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		shaderStageCI.pName = "main";

		VkComputePipelineCreateInfo computePipelineCreateInfo{};
		computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		computePipelineCreateInfo.layout = pipelineLayout;
		computePipelineCreateInfo.flags = 0;
		computePipelineCreateInfo.stage = shaderStageCI;
		vkCheck(vkCreateComputePipelines(RHI::get()->getDevice(), nullptr, 1, &computePipelineCreateInfo, nullptr, &pipeline));
	}

	void SDSMPass::CascadeCulling::release()
	{
		CHECK(pipeline != VK_NULL_HANDLE && pipelineLayout != VK_NULL_HANDLE);
		vkDestroyPipeline(RHI::get()->getDevice(), pipeline, nullptr);
		vkDestroyPipelineLayout(RHI::get()->getDevice(), pipelineLayout, nullptr);
		pipeline = VK_NULL_HANDLE;
		pipelineLayout = VK_NULL_HANDLE;
	}

	void SDSMPass::CascadeDepth::init(SDSMPass* in)
	{
		// create render pass. for traditional sdsm.
		{
			VkAttachmentDescription attachmentDesc{};
			attachmentDesc.format = SceneTextures::getShadowDepthFormat();

			attachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
			attachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkAttachmentReference colorRef{};
			colorRef.attachment = VK_ATTACHMENT_UNUSED;
			colorRef.layout = VK_IMAGE_LAYOUT_UNDEFINED;

			VkAttachmentReference depthRef{};
			depthRef.attachment = 0;
			depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &colorRef;
			subpass.pDepthStencilAttachment = &depthRef;

			std::array<VkSubpassDependency, 2> dependencies{};
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

			VkRenderPassCreateInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.attachmentCount = 1;
			renderPassInfo.pAttachments = &attachmentDesc;
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;
			renderPassInfo.dependencyCount = (uint32_t)dependencies.size();
			renderPassInfo.pDependencies = dependencies.data();

			renderpass = RHI::get()->createRenderpass(renderPassInfo);
		}

		// create frame buffer, for traditional sdsm.
		{
			VulkanFrameBufferFactory fbf{};

			// every time create attachment, force shadow depth reallocate.
			fbf.setRenderpass(renderpass)
			   .addAttachment(in->m_renderer->getSceneTextures()->getShadowDepth());

			framebuffer = fbf.create(RHI::get()->getDevice());
		}

		// create pipeline and layout.
		{
			VkPipelineLayoutCreateInfo plci = vkPipelineLayoutCreateInfo();

			VkPushConstantRange pushConstantsRange{};
			pushConstantsRange.offset = 0;
			pushConstantsRange.size = sizeof(CascadeDepthPushConstantsData);
			pushConstantsRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

			plci.pushConstantRangeCount = 1;
			plci.pPushConstantRanges = &pushConstantsRange;

			std::vector<VkDescriptorSetLayout> setLayouts = {
				in->m_textureManager->getBindless()->getSetLayout(),			          // 0
				RHI::get()->getBindlessSampler()->getSetLayout(),						  // 1
				in->m_renderer->getViewDataRing()->getLayout().layout,					  // 2
				in->m_renderer->getFrameDataRing()->getLayout().layout,					  // 3
				in->m_renderer->getObjectDataRing()->getLayout().layout,				  // 4
				in->m_renderer->getMaterialDataRing()->getLayout().layout,				  // 5
				GSDSMRenderResource.indirectCasccadeBuffer.descriptorSetLayout.layout,    // 6
				GSDSMRenderResource.cascadeInfoBuffer.setLayout.layout 				      // 7
			};
			plci.setLayoutCount = (uint32_t)setLayouts.size();
			plci.pSetLayouts = setLayouts.data();
			pipelineLayout = RHI::get()->createPipelineLayout(plci);

			VulkanGraphicsPipelineFactory gpf = {};

			auto* vertShader = RHI::get()->getShader("Shader/Spirv/SDSMDepth.vert.spv", true);
			auto* fragShader = RHI::get()->getShader("Shader/Spirv/SDSMDepth.frag.spv", true);

			gpf.shaderStages.clear();
			gpf.shaderStages.push_back(vkPipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, *vertShader));
			gpf.shaderStages.push_back(vkPipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, *fragShader));

			VulkanVertexInputDescription vvid = {};
			vvid.bindings = { VulkanVertexBuffer::getInputBinding(getStandardMeshAttributes()) };
			vvid.attributes = VulkanVertexBuffer::getInputAttribute(getStandardMeshAttributes());

			gpf.vertexInputDescription = vvid;
			gpf.inputAssembly = vkInputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
			gpf.rasterizer = vkRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);

			gpf.rasterizer.cullMode = VK_CULL_MODE_NONE;

			gpf.rasterizer.depthBiasEnable = VK_TRUE;
			gpf.rasterizer.depthClampEnable = cVarShadowEnableDepthClamp.get() == 1 ? VK_TRUE : VK_FALSE;
			gpf.multisampling = vkMultisamplingStateCreateInfo();
			gpf.depthStencil = vkDepthStencilCreateInfo(true, true, VK_COMPARE_OP_GREATER);
			gpf.pipelineLayout = pipelineLayout;

			const auto ShadowDepthExtent = in->m_renderer->getSceneTextures()->getShadowDepth()->getExtent();
			VkExtent2D shadowExtent2D = {};

			shadowExtent2D.width = ShadowDepthExtent.width;
			shadowExtent2D.height = ShadowDepthExtent.height;

			VkViewport viewport{};
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;

			viewport.x = .0f;
			viewport.width = (float)shadowExtent2D.width;

			// flip y for mesh render.
			viewport.y = (float)shadowExtent2D.height;
			viewport.height = -(float)shadowExtent2D.height;

			VkRect2D scissor{};
			scissor.offset = { 0,0 };
			scissor.extent = shadowExtent2D;

			gpf.viewport = viewport;
			gpf.scissor = scissor;
			pipeline = gpf.buildMeshDrawPipeline(RHI::get()->getDevice(), renderpass);
		}
	}

	void SDSMPass::CascadeDepth::release()
	{
		CHECK(pipeline != VK_NULL_HANDLE && pipelineLayout != VK_NULL_HANDLE && renderpass != VK_NULL_HANDLE && framebuffer != VK_NULL_HANDLE);

		vkDestroyPipeline(RHI::get()->getDevice(), pipeline, nullptr);
		vkDestroyPipelineLayout(RHI::get()->getDevice(), pipelineLayout, nullptr);

		RHI::get()->destroyRenderpass(renderpass);
		RHI::get()->destroyFramebuffer(framebuffer);

		renderpass = VK_NULL_HANDLE;
		framebuffer = VK_NULL_HANDLE;
		pipeline = VK_NULL_HANDLE;
		pipelineLayout = VK_NULL_HANDLE;
	}

	void SDSMRenderResource::init()
	{
		CHECK(!m_bInitFlag);
		m_bInitFlag = true;

		depthRangeBuffer.init(0);
		cascadeInfoBuffer.init(0);

		indirectCasccadeBuffer.init();
	}

	void SDSMRenderResource::release()
	{
		CHECK(m_bInitFlag);
		m_bInitFlag = false;

		depthRangeBuffer.release();
		cascadeInfoBuffer.release();

		indirectCasccadeBuffer.release();
	}
}