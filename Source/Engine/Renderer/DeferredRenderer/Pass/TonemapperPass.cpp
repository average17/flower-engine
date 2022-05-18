#include "TonemapperPass.h"

namespace flower
{
    void TonemapperPass::initInner()
    {
        createRenderpass();
        createFramebuffer();
        createPipeline();
    }

    void TonemapperPass::releaseInner()
    {
        releaseFramebuffer();
        releaseRenderpass();
        releasePipeline();
    }

    void TonemapperPass::onSceneTextureRecreate(uint32_t width,uint32_t height)
    {
        releaseFramebuffer();
        releasePipeline();

        createFramebuffer();
        createPipeline();
    }

    void TonemapperPass::dynamicRender(VkCommandBuffer cmd)
    {

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

        setPerfMarkerBegin(cmd, "Tonemapper", {1.0f, 0.2f, 0.4f, 1.0f});
        vkCmdBeginRenderPass(cmd,&rpInfo,VK_SUBPASS_CONTENTS_INLINE);
        {
            vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,m_pipeline);

            std::vector<VkDescriptorSet> meshPassSets = {
                m_tonemapperPassDescriptorSet.set
            };

            vkCmdBindDescriptorSets(
                cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,
                m_pipelineLayout,
                0,
                (uint32_t)meshPassSets.size(), meshPassSets.data(),
                0,nullptr
            );

            vkCmdDraw(cmd, 3, 1, 0, 0);
        }

        vkCmdEndRenderPass(cmd);
        setPerfMarkerEnd(cmd);
    }

    void TonemapperPass::createFramebuffer()
    {
        CHECK(m_renderpass != VK_NULL_HANDLE);
        CHECK(m_framebuffer == VK_NULL_HANDLE && "ensure frame buffer release before create.");

        auto extent = m_renderer->getSceneTextures()->getLdrSceneColor()->getExtent();

        VkExtent2D extent2D{};
        extent2D.width = extent.width;
        extent2D.height = extent.height;

        VulkanFrameBufferFactory fbf {};
        fbf.setRenderpass(m_renderpass)
            .addAttachment(m_renderer->getSceneTextures()->getLdrSceneColor());
        m_framebuffer = fbf.create(RHI::get()->getDevice());
    }

    void TonemapperPass::createRenderpass()
    {
        CHECK(m_renderpass == VK_NULL_HANDLE);

        VkAttachmentDescription colorAttachment = {};

        colorAttachment.format =  SceneTextures::getLdrSceneColorFormat();
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
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

    void TonemapperPass::createPipeline()
    {
        CHECK(m_renderpass != VK_NULL_HANDLE);

        // create lighting pass self descriptor set.
        {
            VkDescriptorImageInfo hdrImage = {};
            hdrImage.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            hdrImage.imageView = m_renderer->getSceneTextures()->getHdrSceneColor()->getImageView();
            hdrImage.sampler = RHI::get()->getPointClampEdgeSampler();

            RHI::get()->vkDescriptorFactoryBegin()
                .bindImage(0,&hdrImage,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                .build(m_tonemapperPassDescriptorSet,m_tonemapperPassDescriptorSetLayout);
        }

        VkPipelineLayoutCreateInfo plci = vkPipelineLayoutCreateInfo();
        std::vector<VkDescriptorSetLayout> setLayouts = 
        {
            m_tonemapperPassDescriptorSetLayout.layout
        };

        plci.setLayoutCount = (uint32_t)setLayouts.size();
        plci.pSetLayouts    = setLayouts.data();

        m_pipelineLayout = RHI::get()->createPipelineLayout(plci);

        VulkanGraphicsPipelineFactory gpf = {};

        auto* vertShader = RHI::get()->getShader("Shader/Spirv/FullScreen.vert.spv",false);
        auto* fragShader = RHI::get()->getShader("Shader/Spirv/Tonemapper.frag.spv",true);

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
        sceneTextureExtent2D.width = sceneTextureExtent.width;
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

    void TonemapperPass::releasePipeline()
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

    void TonemapperPass::releaseRenderpass()
    {
        if(m_renderpass != VK_NULL_HANDLE)
        {
            RHI::get()->destroyRenderpass(m_renderpass);
            m_renderpass = VK_NULL_HANDLE;
        }
    }

    void TonemapperPass::releaseFramebuffer()
    {
        if(m_framebuffer != VK_NULL_HANDLE)
        {
            RHI::get()->destroyFramebuffer(m_framebuffer);
            m_framebuffer = VK_NULL_HANDLE;
        }
    }
}