#include "CullingPass.h"

namespace flower
{
    struct GPUCullingPushConstants 
    {
        uint32_t drawCount;
    };

	void CullingPass::createPipeline()
	{
        {

        }


        VkPipelineLayoutCreateInfo plci = vkPipelineLayoutCreateInfo();

        VkPushConstantRange push_constant{};
        push_constant.offset = 0;
        push_constant.size = sizeof(GPUCullingPushConstants);
        push_constant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        plci.pPushConstantRanges = &push_constant;
        plci.pushConstantRangeCount = 1; 

        std::vector<VkDescriptorSetLayout> setLayouts = {
              m_renderer->getObjectDataRing()->getLayout().layout
            , m_renderer->getDrawIndirectBuffer()->descriptorSetLayout.layout
            , m_renderer->getDrawIndirectBuffer()->countDescriptorSetLayout.layout
            , m_renderer->getViewDataRing()->getLayout().layout 
            , m_renderer->getFrameDataRing()->getLayout().layout
        };

        plci.setLayoutCount = (uint32_t)setLayouts.size();
        plci.pSetLayouts = setLayouts.data();

        m_pipelineLayout = RHI::get()->createPipelineLayout(plci);

        auto* shaderModule = RHI::get()->getShader("Shader/Spirv/Culling.comp.spv",true);
        VkPipelineShaderStageCreateInfo shaderStageCI{};
        shaderStageCI.module =  shaderModule->GetModule();
        shaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageCI.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        shaderStageCI.pName = "main";

        VkComputePipelineCreateInfo computePipelineCreateInfo{};
        computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        computePipelineCreateInfo.layout = m_pipelineLayout;
        computePipelineCreateInfo.flags = 0;
        computePipelineCreateInfo.stage = shaderStageCI;
        vkCheck(vkCreateComputePipelines(RHI::get()->getDevice(), nullptr, 1, &computePipelineCreateInfo, nullptr, &m_pipeline));
	}

	void CullingPass::releasePipeline()
	{
        vkDestroyPipeline(RHI::get()->getDevice(),m_pipeline,nullptr);
        vkDestroyPipelineLayout(RHI::get()->getDevice(),m_pipelineLayout,nullptr);
	}

    void CullingPass::initInner()
    {
        createPipeline();
    }

    void CullingPass::onSceneTextureRecreate(uint32_t width,uint32_t height)
    {
        releasePipeline();
        createPipeline();
    }

    void CullingPass::releaseInner()
    {
        releasePipeline();
    }

    void CullingPass::gbufferCulling(VkCommandBuffer cmd)
    {
        setPerfMarkerBegin(cmd, "GBufferCulling", {1.0f, 0.0f, 0.0f, 1.0f});
        GPUCullingPushConstants gpuPushConstant = {};
        gpuPushConstant.drawCount = (uint32_t)m_renderer->getRenderScene()->getMeshObjectSSBOData().size();

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
        vkCmdPushConstants(cmd, m_pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(GPUCullingPushConstants), &gpuPushConstant);

        std::vector<VkDescriptorSet> compPassSets = 
        {
            m_renderer->getObjectDataRing()->getSet().set
            , m_renderer->getDrawIndirectBuffer()->descriptorSets.set
            , m_renderer->getDrawIndirectBuffer()->countDescriptorSets.set
            , m_renderer->getViewDataRing()->getSet().set
            , m_renderer->getFrameDataRing()->getSet().set
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

        vkCmdDispatch(cmd, (gpuPushConstant.drawCount / 256) + 1, 1, 1);
        setPerfMarkerEnd(cmd);
    }
}