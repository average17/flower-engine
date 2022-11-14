#include "Pch.h"
#include "../DeferredRenderer.h"
#include "../../Renderer.h"
#include "../../RenderSceneData.h"
#include "../../SceneTextures.h"
#include "../../RenderSettingContext.h"


namespace Flower
{
    struct TonemapperPushComposite
    {
        glm::vec4 prefilterFactor;
        float bloomIntensity;
        float bloomBlur;
    };

    class TonemapperPass : public PassInterface
    {
    public:
        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout setLayout = VK_NULL_HANDLE;

    public:
        virtual void init() override
        {
            CHECK(pipeline == VK_NULL_HANDLE);
            CHECK(pipelineLayout == VK_NULL_HANDLE);
            CHECK(setLayout == VK_NULL_HANDLE);

            // Config code.
            RHI::get()->descriptorFactoryBegin()
                .bindNoInfo(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 0) // inHdr
                .bindNoInfo(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 1) // outLdr
                .bindNoInfo(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 2) // inBloom
                .bindNoInfo(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 3) // inLum
                .buildNoInfoPush(setLayout);

            std::vector<VkDescriptorSetLayout> setLayouts =
            {
                setLayout, // Owner setlayout.
                RHI::SamplerManager->getCommonDescriptorSetLayout(),
                GetLayoutStatic(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER),  // viewData
                GetLayoutStatic(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER),  // frameData
            };
            auto shaderModule = RHI::ShaderManager->getShader("Tonemapper.comp.spv", true);

            // Vulkan buid functions.
            VkPipelineLayoutCreateInfo plci = RHIPipelineLayoutCreateInfo();
            VkPushConstantRange pushRange{ .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT, .offset = 0, .size = sizeof(TonemapperPushComposite) };
            plci.pushConstantRangeCount = 1;
            plci.pPushConstantRanges = &pushRange;

            plci.setLayoutCount = (uint32_t)setLayouts.size();
            plci.pSetLayouts = setLayouts.data();
            pipelineLayout = RHI::get()->createPipelineLayout(plci);
            VkPipelineShaderStageCreateInfo shaderStageCI{};
            shaderStageCI.module = shaderModule;
            shaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStageCI.stage = VK_SHADER_STAGE_COMPUTE_BIT;
            shaderStageCI.pName = "main";
            VkComputePipelineCreateInfo computePipelineCreateInfo{};
            computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
            computePipelineCreateInfo.layout = pipelineLayout;
            computePipelineCreateInfo.flags = 0;
            computePipelineCreateInfo.stage = shaderStageCI;
            RHICheck(vkCreateComputePipelines(RHI::Device, nullptr, 1, &computePipelineCreateInfo, nullptr, &pipeline));
        }

        virtual void release() override
        {
            RHISafeRelease(pipeline);
            RHISafeRelease(pipelineLayout);
            setLayout = VK_NULL_HANDLE;
        }
    };

    void DeferredRenderer::renderTonemapper(
        VkCommandBuffer cmd, 
        Renderer* renderer, 
        SceneTextures* inTextures, 
        RenderSceneData* scene, 
        BufferParamRefPointer& viewData,
        BufferParamRefPointer& frameData,
        PoolImageSharedRef bloomTex)
    {
        auto& hdrSceneColor = inTextures->getHdrSceneColorUpscale()->getImage();
        auto& ldrSceneColor = getDisplayOutput();
        {
            hdrSceneColor.transitionLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, buildBasicImageSubresource());
            ldrSceneColor.transitionLayout(cmd, VK_IMAGE_LAYOUT_GENERAL, buildBasicImageSubresource());
        }

        {
            RHI::ScopePerframeMarker tonemapperMarker(cmd, "Tonemapper", { 1.0f, 1.0f, 0.0f, 1.0f });

            auto* pass = getPasses()->getPass<TonemapperPass>();

            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pass->pipeline);

            VkDescriptorImageInfo hdrImageInfo = RHIDescriptorImageInfoSample(hdrSceneColor.getView(buildBasicImageSubresource()));
            VkDescriptorImageInfo ldrImageInfo = RHIDescriptorImageInfoStorage(ldrSceneColor.getView(buildBasicImageSubresource()));

            auto inRange = VkImageSubresourceRange
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            };

            // Input from low res mip. upscale mip.
            VkDescriptorImageInfo bloomImgInfo = RHIDescriptorImageInfoSample(bloomTex->getImage().getView(inRange));

            CHECK(m_averageLum && "Must call adaptive exposure before tonemapper."); // TODO: 
            VkDescriptorImageInfo lumImgInfo = RHIDescriptorImageInfoSample(m_averageLum->getImage().getView(inRange));

            std::vector<VkWriteDescriptorSet> writes
            {
                RHIPushWriteDescriptorSetImage(0, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, &hdrImageInfo),
                RHIPushWriteDescriptorSetImage(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &ldrImageInfo),
                RHIPushWriteDescriptorSetImage(2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, &bloomImgInfo),
                RHIPushWriteDescriptorSetImage(3, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, &lumImgInfo),
            };

            // Push owner set #0.
            RHI::PushDescriptorSetKHR(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pass->pipelineLayout, 0, uint32_t(writes.size()), writes.data());

            std::vector<VkDescriptorSet> passSets =
            {
                RHI::SamplerManager->getCommonDescriptorSet(), // samplers.
                viewData->buffer.getSet(),
                frameData->buffer.getSet(),
            };
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pass->pipelineLayout,
                1, (uint32_t)passSets.size(), passSets.data(), 0, nullptr);

            TonemapperPushComposite compositePush
            {
                .bloomIntensity = RenderSettingManager::get()->bloomIntensity,
                .bloomBlur = RenderSettingManager::get()->bloomRadius
            };

            float knee = RenderSettingManager::get()->bloomThreshold * RenderSettingManager::get()->bloomThresholdSoft;

            compositePush.prefilterFactor.x = RenderSettingManager::get()->bloomThreshold;
            compositePush.prefilterFactor.y = compositePush.prefilterFactor.x - knee;
            compositePush.prefilterFactor.z = 2.0f * knee;
            compositePush.prefilterFactor.w = 0.25f / (knee + 0.00001f);

            vkCmdPushConstants(cmd, pass->pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(compositePush), &compositePush);


            vkCmdDispatch(cmd, getGroupCount(ldrSceneColor.getExtent().width, 8), getGroupCount(ldrSceneColor.getExtent().height, 8), 1);

            m_gpuTimer.getTimeStamp(cmd, "Tonemappering");
        }


    }
}