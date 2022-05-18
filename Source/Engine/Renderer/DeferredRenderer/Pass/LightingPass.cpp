#include "LightingPass.h"
#include "SDSMPass.h"
#include "../../../Scene/Component/AtmosphereSky.h"

namespace flower
{
    struct LightingPushConstant
    {
        uint32_t cascadeCount;
        uint32_t shadowMapPageDimXY;
        uint32_t validAtmosphere = 0;
        float groundRadiusMM = 6.360f;
        float atmosphereRadiusMM = 6.460f;
        float offsetHeight = 0.0002f;
    };

    void LightingPass::initInner()
    {
        createRenderpass();
        createFramebuffer();
        createPipeline();
    }

    void LightingPass::releaseInner()
    {
        releaseFramebuffer();
        releaseRenderpass();
        releasePipeline();
    }

    void LightingPass::onSceneTextureRecreate(uint32_t width,uint32_t height)
    {
        releaseFramebuffer();
        releasePipeline();

        createFramebuffer();
        createPipeline();
    }

    void LightingPass::dynamicRender(VkCommandBuffer cmd)
    {
        LightingPushConstant pushConst{};

        auto atmosphereSky = m_renderer->getRenderScene()->getAtmosphereSky();
        if (atmosphereSky)
        {
            AtmosphereParameters pushConstants = atmosphereSky->params;

            pushConst.atmosphereRadiusMM = pushConstants.atmosphereRadiusMM;
            pushConst.groundRadiusMM = pushConstants.groundRadiusMM;
            pushConst.offsetHeight = pushConstants.offsetHeight;
            pushConst.validAtmosphere = 1;
        }

        auto sceneTextureExtent = m_renderer->getSceneTextures()->getDepthStencil()->getExtent();

        VkExtent2D sceneTextureExtent2D{};
        sceneTextureExtent2D.width = sceneTextureExtent.width;
        sceneTextureExtent2D.height = sceneTextureExtent.height;

        VkRenderPassBeginInfo rpInfo = vkRenderpassBeginInfo(m_renderpass,sceneTextureExtent2D,m_framebuffer);

        VkClearValue colorValue;
        colorValue.color = {{ 0.0f, 0.0f, 0.0f, 0.0f }};

        VkClearValue depthClear;
        depthClear.depthStencil.depth = 1.f;
        depthClear.depthStencil.stencil = 1;

        VkClearValue clearValues[2] = {colorValue, depthClear};
        rpInfo.clearValueCount = 2;
        rpInfo.pClearValues = &clearValues[0];

        VkRect2D scissor{};
        scissor.extent = sceneTextureExtent2D;
        scissor.offset = {0,0};
        
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)sceneTextureExtent2D.width;
        viewport.height = (float)sceneTextureExtent2D.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        vkCmdSetViewport(cmd,0,1,&viewport);
        vkCmdSetDepthBias(cmd,0,0,0);
        vkCmdSetScissor(cmd,0,1,&scissor);

        setPerfMarkerBegin(cmd, "Lighting", {1.0f, 0.2f, 0.1f, 1.0f});
        vkCmdBeginRenderPass(cmd,&rpInfo,VK_SUBPASS_CONTENTS_INLINE);
        {
            vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,m_pipeline);

            std::vector<VkDescriptorSet> meshPassSets = {
                  m_renderer->getFrameDataRing()->getSet().set
                , m_renderer->getViewDataRing()->getSet().set
                , GSDSMRenderResource.cascadeInfoBuffer.set.set
                , m_lightingPassDescriptorSet.set
            };

            vkCmdBindDescriptorSets(
                cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,
                m_pipelineLayout,
                0,
                (uint32_t)meshPassSets.size(), meshPassSets.data(),
                0,nullptr
            );
            
           
            pushConst.cascadeCount = getShadowCascadeCount();
            pushConst.shadowMapPageDimXY = getShadowPerPageDimXY();

            vkCmdPushConstants(cmd, m_pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(LightingPushConstant), &pushConst);

            vkCmdDraw(cmd, 3, 1, 0, 0);
        }
        
        vkCmdEndRenderPass(cmd);
        setPerfMarkerEnd(cmd);
    }

    void LightingPass::createFramebuffer()
    {
        CHECK(m_renderpass != VK_NULL_HANDLE);
        CHECK(m_framebuffer == VK_NULL_HANDLE && "ensure frame buffer release before create.");

        auto extent = m_renderer->getSceneTextures()->getHdrSceneColor()->getExtent();

        VkExtent2D extent2D{};
        extent2D.width = extent.width;
        extent2D.height = extent.height;

        VulkanFrameBufferFactory fbf {};
        fbf.setRenderpass(m_renderpass)
            .addAttachment(m_renderer->getSceneTextures()->getHdrSceneColor());
        m_framebuffer = fbf.create(RHI::get()->getDevice());
    }

    void LightingPass::createRenderpass()
    {
        CHECK(m_renderpass == VK_NULL_HANDLE);

        VkAttachmentDescription colorAttachment = {};

        colorAttachment.format =  SceneTextures::getHdrSceneColorFormat();
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = nullptr;

        std::array<VkSubpassDependency, 2> dependencies;
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

        VkRenderPassCreateInfo render_pass_info = {};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        render_pass_info.attachmentCount = 1;
        render_pass_info.pAttachments = &colorAttachment;
        render_pass_info.subpassCount = 1;
        render_pass_info.pSubpasses = &subpass;
        render_pass_info.dependencyCount = 2;
        render_pass_info.pDependencies = dependencies.data();

        m_renderpass = RHI::get()->createRenderpass(render_pass_info);
        setResourceName(VK_OBJECT_TYPE_RENDER_PASS,(uint64_t)m_renderpass, getPassName().c_str());
    }

    void LightingPass::createPipeline()
    {
        CHECK(m_renderpass != VK_NULL_HANDLE);

        // create lighting pass self descriptor set.
        {
            VkDescriptorImageInfo gbufferAImage = {};
            gbufferAImage.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            gbufferAImage.imageView = m_renderer->getSceneTextures()->getGbufferA()->getImageView();
            gbufferAImage.sampler = RHI::get()->getPointClampEdgeSampler();

            VkDescriptorImageInfo gbufferBImage = {};
            gbufferBImage.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            gbufferBImage.imageView = m_renderer->getSceneTextures()->getGbufferB()->getImageView();
            gbufferBImage.sampler = RHI::get()->getPointClampEdgeSampler();

            VkDescriptorImageInfo gbufferCImage = {};
            gbufferCImage.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            gbufferCImage.imageView = m_renderer->getSceneTextures()->getGbufferC()->getImageView();
            gbufferCImage.sampler = RHI::get()->getPointClampEdgeSampler();

            VkDescriptorImageInfo gbufferSImage = {};
            gbufferSImage.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            gbufferSImage.imageView = m_renderer->getSceneTextures()->getGbufferS()->getImageView();
            gbufferSImage.sampler = RHI::get()->getPointClampEdgeSampler();

            VkDescriptorImageInfo depthStencilImage = {};
            depthStencilImage.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            depthStencilImage.imageView = m_renderer->getSceneTextures()->getDepthStencil()->getDepthOnlyImageView();
            depthStencilImage.sampler = RHI::get()->getLinearClampNoMipSampler();

            VkDescriptorImageInfo shadowImage = {};
            shadowImage.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            shadowImage.imageView = m_renderer->getSceneTextures()->getShadowDepth()->getImageView();
            shadowImage.sampler = RHI::get()->getShadowFilterSampler();

            VkDescriptorImageInfo brdfLut = {};
            brdfLut.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            brdfLut.imageView = m_renderer->getStaticTextures()->getBRDFLut()->getImageView();
            brdfLut.sampler = RHI::get()->getPointClampEdgeSampler();

            VkDescriptorImageInfo envCapture = {};
            envCapture.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            envCapture.imageView = m_renderer->getStaticTextures()->getFallbackReflectionCapture()->getImageView();
            envCapture.sampler = RHI::get()->getLinearClampSampler();

            VkDescriptorImageInfo ssrTexture = {};
            ssrTexture.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            ssrTexture.imageView = m_renderer->getSceneTextures()->getSSRTemporalFilter()->getImageView();
            ssrTexture.sampler = RHI::get()->getLinearClampSampler();

            VkDescriptorImageInfo ssaoTexture = {};
            ssaoTexture.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            ssaoTexture.imageView = m_renderer->getSceneTextures()->getSSAOMask()->getImageView();
            ssaoTexture.sampler = RHI::get()->getLinearClampSampler();


            VkDescriptorImageInfo inTransimittanceTexture = {};
            inTransimittanceTexture.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            inTransimittanceTexture.imageView = m_renderer->getStaticTextures()->getEpicAtmosphere()->transmittanceLut->getImageView();
            inTransimittanceTexture.sampler = RHI::get()->getLinearClampSampler();

            RHI::get()->vkDescriptorFactoryBegin()
                .bindImage(0,&gbufferAImage,    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                .bindImage(1,&gbufferBImage,    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                .bindImage(2,&gbufferCImage,    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                .bindImage(3,&gbufferSImage,    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                .bindImage(4,&depthStencilImage,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                .bindImage(5,&shadowImage,      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                .bindImage(6,&brdfLut,          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                .bindImage(7,&envCapture,       VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                .bindImage(8,&ssrTexture,       VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                .bindImage(9,&ssaoTexture,      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                .bindImage(10,&inTransimittanceTexture, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                .build(m_lightingPassDescriptorSet, m_lightingPassDescriptorSetLayout);
        }

        VkPipelineLayoutCreateInfo plci = vkPipelineLayoutCreateInfo();
        plci.pushConstantRangeCount = 1;

        VkPushConstantRange pushRange{};
        pushRange.offset = 0;
        pushRange.size = sizeof(LightingPushConstant);
        pushRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        plci.pPushConstantRanges = &pushRange;

        std::vector<VkDescriptorSetLayout> setLayouts = 
        {
            m_renderer->getFrameDataRing()->getLayout().layout,
            m_renderer->getViewDataRing()->getLayout().layout,
            GSDSMRenderResource.cascadeInfoBuffer.setLayout.layout,
            m_lightingPassDescriptorSetLayout.layout
        };

        plci.setLayoutCount = (uint32_t)setLayouts.size();
        plci.pSetLayouts    = setLayouts.data();

        m_pipelineLayout = RHI::get()->createPipelineLayout(plci);

        VulkanGraphicsPipelineFactory gpf = {};

        auto* vertShader = RHI::get()->getShader("Shader/Spirv/FullScreen.vert.spv",false);
        auto* fragShader = RHI::get()->getShader("Shader/Spirv/Lighting.frag.spv",true);

        gpf.shaderStages.clear();
        gpf.shaderStages.push_back(vkPipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, *vertShader));
        gpf.shaderStages.push_back(vkPipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, *fragShader));

        gpf.vertexInputInfo = vkVertexInputStateCreateInfo();

        gpf.vertexInputInfo.vertexBindingDescriptionCount = 0;
        gpf.vertexInputInfo.pVertexBindingDescriptions = nullptr;
        gpf.vertexInputInfo.vertexAttributeDescriptionCount = 0;
        gpf.vertexInputInfo.pVertexAttributeDescriptions = nullptr;

        gpf.inputAssembly = vkInputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        gpf.depthStencil = vkDepthStencilCreateInfo(false, false, VK_COMPARE_OP_ALWAYS);

        auto sceneTextureExtent = m_renderer->getSceneTextures()->getHdrSceneColor()->getExtent();
        VkExtent2D sceneTextureExtent2D{};
        sceneTextureExtent2D.width  = sceneTextureExtent.width;
        sceneTextureExtent2D.height = sceneTextureExtent.height;

        gpf.viewport.x = 0.0f;
        gpf.viewport.y = 0.0f;
        gpf.viewport.width =  (float)sceneTextureExtent2D.width;
        gpf.viewport.height = (float)sceneTextureExtent2D.height;
        gpf.viewport.minDepth = 0.0f;
        gpf.viewport.maxDepth = 1.0f;
        gpf.scissor.offset = { 0, 0 };
        gpf.scissor.extent = sceneTextureExtent2D;

        gpf.rasterizer = vkRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);
        gpf.rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
        gpf.rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

        gpf.multisampling = vkMultisamplingStateCreateInfo();
        gpf.colorBlendAttachments = { vkColorBlendAttachmentState()};
        gpf.pipelineLayout = m_pipelineLayout;

        m_pipeline = gpf.buildMeshDrawPipeline(RHI::get()->getDevice(), m_renderpass);
    }

    void LightingPass::releasePipeline()
    {
        if(m_pipeline!=VK_NULL_HANDLE)
        {
            vkDestroyPipeline(RHI::get()->getDevice(),m_pipeline,nullptr);
            m_pipeline = VK_NULL_HANDLE;
        }

        if(m_pipelineLayout!=VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(RHI::get()->getDevice(),m_pipelineLayout,nullptr);
            m_pipelineLayout = VK_NULL_HANDLE;
        }
    }

    void LightingPass::releaseRenderpass()
    {
        if(m_renderpass != VK_NULL_HANDLE)
        {
            RHI::get()->destroyRenderpass(m_renderpass);
            m_renderpass = VK_NULL_HANDLE;
        }
    }

    void LightingPass::releaseFramebuffer()
    {
        if(m_framebuffer != VK_NULL_HANDLE)
        {
            RHI::get()->destroyFramebuffer(m_framebuffer);
            m_framebuffer = VK_NULL_HANDLE;
        }
    }
}