#include "GBufferPass.h"
#include "../../Mesh.h"

#include "../../../Asset/AssetMesh.h"
#include "../../../Asset/AssetSystem.h"
#include "../../../Asset/AssetTexture.h"
#include "../../../Engine.h"

namespace flower
{
    void GBufferPass::initInner()
    {
        m_meshManager = GEngine.getRuntimeModule<AssetSystem>()->getMeshManager();
        m_textureManager = GEngine.getRuntimeModule<AssetSystem>()->getTextureManager();
        createRenderpass();
        createFramebuffer();
        createFallbackPipeline();
    }

    void GBufferPass::releaseInner()
    {
        releaseFramebuffer();
        releaseRenderpass();
        releaseFallbackPipeline();
    }

    void GBufferPass::onSceneTextureRecreate(uint32_t width,uint32_t height)
    {
        releaseFramebuffer();
        releaseFallbackPipeline(); // for hot reload we also recreate fallback pipeline when viewport resize.

        createFramebuffer();
        createFallbackPipeline();
    }

	void GBufferPass::dynamicRender(VkCommandBuffer cmd)
	{
        auto extent = m_renderer->getSceneTextures()->getDepthStencil()->getExtent();

        VkExtent2D sceneTextureExtent2D{};
        sceneTextureExtent2D.width  = extent.width;
        sceneTextureExtent2D.height = extent.height;

        setPerfMarkerBegin(cmd, "GBuffer", {1.0f, 0.0f, 0.0f, 1.0f});
        VkRenderPassBeginInfo rpInfo = vkRenderpassBeginInfo(m_renderpass,sceneTextureExtent2D,m_framebuffer);

        std::array<VkClearValue,6> clearValues{};

        clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } }; // Gbuffer A
        clearValues[1].color = { { 0.0f, 0.0f, 0.0f, 0.0f } }; // Gbuffer B
        clearValues[2].color = { { 0.0f, 0.0f, 0.0f, 0.0f } }; // Gbuffer C
        clearValues[3].color = { { 0.0f, 0.0f, 0.0f, 0.0f } }; // Gbuffer S
        clearValues[4].color = { { 0.0f, 0.0f, 0.0f, 0.0f } }; // Velocity
        clearValues[5].depthStencil = { 0.0f, 1 }; // Depth Stencil

        rpInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        rpInfo.pClearValues = &clearValues[0];

        VkRect2D scissor{};
        scissor.extent = sceneTextureExtent2D;
        scissor.offset = {0,0};

        // flip y on mesh raster
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = (float)sceneTextureExtent2D.height; 
        viewport.width  = (float)sceneTextureExtent2D.width;
        viewport.height = -(float)sceneTextureExtent2D.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        m_meshManager->bindIndexBuffer(cmd);
        m_meshManager->bindVertexBuffer(cmd);

        // barrier for gbuffer culling pass.
        VkDeviceSize MeshSSBOSize = (uint32_t)m_renderer->getRenderScene()->getMeshObjectSSBOData().size();
        MeshSSBOSize = MeshSSBOSize == 0 ? VK_WHOLE_SIZE : MeshSSBOSize;

        std::array<VkBufferMemoryBarrier,2> barriers{};
        barriers[0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barriers[0].buffer = m_renderer->getDrawIndirectBuffer()->drawIndirectSSBO->getVkBuffer();
        barriers[0].size = MeshSSBOSize;
        barriers[0].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barriers[0].dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        barriers[0].srcQueueFamilyIndex = RHI::get()->getVulkanDevice()->graphicsFamily;
        barriers[0].dstQueueFamilyIndex = RHI::get()->getVulkanDevice()->graphicsFamily;

        barriers[1].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barriers[1].buffer = m_renderer->getDrawIndirectBuffer()->countBuffer->getVkBuffer();
        barriers[1].size = m_renderer->getDrawIndirectBuffer()->countSize;
        barriers[1].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barriers[1].dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        barriers[1].srcQueueFamilyIndex = RHI::get()->getVulkanDevice()->graphicsFamily;
        barriers[1].dstQueueFamilyIndex = RHI::get()->getVulkanDevice()->graphicsFamily;

        vkCmdPipelineBarrier(
            cmd,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
            0,
            0,nullptr,
            (uint32_t)barriers.size(),
            barriers.data(),
            0,
            nullptr
        );

        vkCmdBeginRenderPass(cmd,&rpInfo,VK_SUBPASS_CONTENTS_INLINE);
        {
            uint32_t countObject = (uint32_t)m_renderer->getRenderScene()->getMeshObjectSSBOData().size();

            if(countObject>0)
            {
                vkCmdSetScissor(cmd,0,1,&scissor);
                vkCmdSetViewport(cmd,0,1,&viewport);
                vkCmdSetDepthBias(cmd,0,0,0);

                std::vector<VkDescriptorSet> meshPassSets = {
                    m_textureManager->getBindless()->getSet(), //0
                    RHI::get()->getBindlessSampler()->getSet(), // 1
                    m_renderer->getViewDataRing()->getSet().set, // 2
                    m_renderer->getFrameDataRing()->getSet().set, // 3
                    m_renderer->getObjectDataRing()->getSet().set, // 4
                    m_renderer->getMaterialDataRing()->getSet().set,//5
                    m_renderer->getDrawIndirectBuffer()->descriptorSets.set //6
                };

                vkCmdBindDescriptorSets(
                    cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,
                    m_pipelineLayout,
                    0,
                    (uint32_t)meshPassSets.size(),
                    meshPassSets.data(),
                    0,
                    nullptr
                );
                vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,m_pipeline);

                vkCmdDrawIndexedIndirectCount(
                    cmd,
                    m_renderer->getDrawIndirectBuffer()->drawIndirectSSBO->getVkBuffer(),
                    0,
                    m_renderer->getDrawIndirectBuffer()->countBuffer->getVkBuffer(),
                    0,
                    countObject,
                    sizeof(GPUDrawCallData)
                );
            }
            
        }
        vkCmdEndRenderPass(cmd); 
        setPerfMarkerEnd(cmd);
	}

	void GBufferPass::createFramebuffer()
	{
        CHECK(m_renderpass != VK_NULL_HANDLE);
		CHECK(m_framebuffer == VK_NULL_HANDLE && "ensure frame buffer release before create.");

        auto extent = m_renderer->getSceneTextures()->getDepthStencil()->getExtent();

        VkExtent2D extent2D{};
        extent2D.width = extent.width;
        extent2D.height = extent.height;

        VulkanFrameBufferFactory fbf {};
        fbf.setRenderpass(m_renderpass)
           .addAttachment(m_renderer->getSceneTextures()->getGbufferA())
           .addAttachment(m_renderer->getSceneTextures()->getGbufferB())
           .addAttachment(m_renderer->getSceneTextures()->getGbufferC())
           .addAttachment(m_renderer->getSceneTextures()->getGbufferS())
           .addAttachment(m_renderer->getSceneTextures()->getVelocity())
           .addAttachment(m_renderer->getSceneTextures()->getDepthStencil());
        m_framebuffer = fbf.create(RHI::get()->getDevice());
	}

	void GBufferPass::createRenderpass()
	{
		CHECK(m_renderpass == VK_NULL_HANDLE);

        constexpr size_t GBufferA_Index     = 0;
        constexpr size_t GBufferB_Index     = 1;
        constexpr size_t GBufferC_Index     = 2;
        constexpr size_t GBufferS_Index     = 3;
        constexpr size_t Velocity_Index     = 4;
        constexpr size_t DepthStencil_Index = 5;

        constexpr size_t ColorTargetCount   = DepthStencil_Index;
        constexpr size_t RenderTargetCount  = DepthStencil_Index + 1;
        std::array<VkAttachmentDescription, RenderTargetCount> attachmentDescs{};

        // Gbuffer A
        {
            constexpr auto id = GBufferA_Index;
            attachmentDescs[id] = {};
            attachmentDescs[id].format         = SceneTextures::getGbufferAFormat();
            attachmentDescs[id].samples        = VK_SAMPLE_COUNT_1_BIT;
            attachmentDescs[id].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachmentDescs[id].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
            attachmentDescs[id].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachmentDescs[id].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachmentDescs[id].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
            attachmentDescs[id].finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }

        // Gbuffer B
        {
            constexpr auto id = GBufferB_Index;
            attachmentDescs[id] = {};
            attachmentDescs[id].format         = SceneTextures::getGbufferBFormat();
            attachmentDescs[id].samples        = VK_SAMPLE_COUNT_1_BIT;
            attachmentDescs[id].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachmentDescs[id].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
            attachmentDescs[id].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachmentDescs[id].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachmentDescs[id].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
            attachmentDescs[id].finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }

        // Gbuffer C
        {
            constexpr auto id = GBufferC_Index;
            attachmentDescs[id] = {};
            attachmentDescs[id].format         = SceneTextures::getGbufferCFormat();
            attachmentDescs[id].samples        = VK_SAMPLE_COUNT_1_BIT;
            attachmentDescs[id].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachmentDescs[id].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
            attachmentDescs[id].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachmentDescs[id].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachmentDescs[id].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
            attachmentDescs[id].finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }

        // Gbuffer S
        {
            constexpr auto id = GBufferS_Index;
            attachmentDescs[id] = {};
            attachmentDescs[id].format         = SceneTextures::getGbufferSFormat();
            attachmentDescs[id].samples        = VK_SAMPLE_COUNT_1_BIT;
            attachmentDescs[id].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachmentDescs[id].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
            attachmentDescs[id].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachmentDescs[id].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachmentDescs[id].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
            attachmentDescs[id].finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }

        // Velocity
        {
            constexpr auto id = Velocity_Index;
            attachmentDescs[id] = {};
            attachmentDescs[id].format = SceneTextures::getVelocityFormat();
            attachmentDescs[id].samples = VK_SAMPLE_COUNT_1_BIT;
            attachmentDescs[id].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachmentDescs[id].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachmentDescs[id].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachmentDescs[id].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachmentDescs[id].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachmentDescs[id].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }
        
        // DepthStencil
        {
            constexpr auto id = DepthStencil_Index;
            attachmentDescs[id] = {};
            attachmentDescs[id].format         = SceneTextures::getDepthStencilFormat();
            attachmentDescs[id].samples        = VK_SAMPLE_COUNT_1_BIT;
            attachmentDescs[id].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachmentDescs[id].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
            attachmentDescs[id].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachmentDescs[id].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachmentDescs[id].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
            attachmentDescs[id].finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }

        std::array<VkAttachmentReference, ColorTargetCount> colorReferences;
        colorReferences[GBufferA_Index] = {GBufferA_Index, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        colorReferences[GBufferB_Index] = {GBufferB_Index, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        colorReferences[GBufferC_Index] = {GBufferC_Index, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        colorReferences[GBufferS_Index] = {GBufferS_Index, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        colorReferences[Velocity_Index] = {Velocity_Index, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

        VkAttachmentReference depthReference = {};
        depthReference.attachment = DepthStencil_Index;
        depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
        subpass.pColorAttachments    = colorReferences.data();
        subpass.pDepthStencilAttachment = &depthReference;

        std::array<VkSubpassDependency, 2> dependencies;
        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask  = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[0].dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].dstStageMask  = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescs.size());
        renderPassInfo.pAttachments = attachmentDescs.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 2;
        renderPassInfo.pDependencies = dependencies.data();

        m_renderpass = RHI::get()->createRenderpass(renderPassInfo);

        setResourceName(VK_OBJECT_TYPE_RENDER_PASS,(uint64_t)m_renderpass, getPassName().c_str());
	}

    void GBufferPass::createFallbackPipeline()
    {
        CHECK(m_renderpass != VK_NULL_HANDLE);

        VkPipelineLayoutCreateInfo plci = vkPipelineLayoutCreateInfo();
        std::vector<VkDescriptorSetLayout> setLayouts = 
        {
            m_textureManager->getBindless()->getSetLayout(), //0
            RHI::get()->getBindlessSampler()->getSetLayout(), // 1
            m_renderer->getViewDataRing()->getLayout().layout, // 2
            m_renderer->getFrameDataRing()->getLayout().layout, // 3
            m_renderer->getObjectDataRing()->getLayout().layout, // 4
            m_renderer->getMaterialDataRing()->getLayout().layout,//5
            m_renderer->getDrawIndirectBuffer()->descriptorSetLayout.layout //6
        };

        plci.pushConstantRangeCount = 0;
        plci.setLayoutCount = (uint32_t)setLayouts.size();
        plci.pSetLayouts    = setLayouts.data();

        m_pipelineLayout = RHI::get()->createPipelineLayout(plci);

        VulkanGraphicsPipelineFactory gpf = {};
 
        auto* vertShader = RHI::get()->getShader("Shader/Spirv/GBuffer.vert.spv",true);
        auto* fragShader = RHI::get()->getShader("Shader/Spirv/GBuffer.frag.spv",true);

        gpf.shaderStages.clear();
        gpf.shaderStages.push_back(vkPipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, *vertShader));
        gpf.shaderStages.push_back(vkPipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, *fragShader));

        VulkanVertexInputDescription vvid = {};
        vvid.bindings   =  { VulkanVertexBuffer::getInputBinding(getStandardMeshAttributes()) };
        vvid.attributes =    VulkanVertexBuffer::getInputAttribute(getStandardMeshAttributes());

        gpf.vertexInputDescription = vvid;
        gpf.inputAssembly = vkInputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        gpf.rasterizer = vkRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);
        gpf.rasterizer.cullMode = VK_CULL_MODE_NONE;
        gpf.multisampling = vkMultisamplingStateCreateInfo();

        gpf.colorBlendAttachments.push_back(vkColorBlendAttachmentState()); // Gbuffer A
        gpf.colorBlendAttachments.push_back(vkColorBlendAttachmentState()); // Gbuffer B
        gpf.colorBlendAttachments.push_back(vkColorBlendAttachmentState()); // Gbuffer C
        gpf.colorBlendAttachments.push_back(vkColorBlendAttachmentState()); // Gbuffer S
        gpf.colorBlendAttachments.push_back(vkColorBlendAttachmentState()); // velocity

        gpf.depthStencil   = vkDepthStencilCreateInfo(true, true, VK_COMPARE_OP_GREATER);
        gpf.pipelineLayout = m_pipelineLayout;

        // Gbuffer flip viewport.
        VkExtent2D sceneTextureExtent2D{};
        sceneTextureExtent2D.width = m_renderer->getRenderWidth();
        sceneTextureExtent2D.height = m_renderer->getRenderHeight();
        gpf.viewport.x = 0.0f;
        gpf.viewport.y = (float)sceneTextureExtent2D.height;
        gpf.viewport.width  =  (float)sceneTextureExtent2D.width;
        gpf.viewport.height = -(float)sceneTextureExtent2D.height;
        gpf.viewport.minDepth = 0.0f;
        gpf.viewport.maxDepth = 1.0f;
        gpf.scissor.offset = { 0, 0 };
        gpf.scissor.extent = sceneTextureExtent2D;

        m_pipeline = gpf.buildMeshDrawPipeline(RHI::get()->getDevice(),m_renderpass);
    }

    void GBufferPass::releaseFallbackPipeline()
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

    void GBufferPass::releaseRenderpass()
    {
        if(m_renderpass != VK_NULL_HANDLE)
        {
            RHI::get()->destroyRenderpass(m_renderpass);
            m_renderpass = VK_NULL_HANDLE;
        }
    }

    void GBufferPass::releaseFramebuffer()
    {
        if(m_framebuffer != VK_NULL_HANDLE)
        {
            RHI::get()->destroyFramebuffer(m_framebuffer);
            m_framebuffer = VK_NULL_HANDLE;
        }
    }
}