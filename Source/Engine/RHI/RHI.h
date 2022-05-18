#pragma once

#include "Error.h"
#include "Instance.h"
#include "Descriptor.h"
#include "Device.h"
#include "Shader.h"
#include "Buffer.h"
#include "Fence.h"
#include "Sampler.h"
#include "SwapChain.h"
#include "CommandBuffer.h"
#include "Factory.h"
#include "Common.h"
#include "ImGuiCommon.h"
#include "Bindless.h"
#include "ObjectUpload.h"

#include "../Core/Delegates.h"

namespace flower{

    class RHI
    {
    private:
        bool m_bOk = false;
        bool m_swapchainChange = false;

    private:
        std::vector<const char*> m_instanceLayerNames = {};
        std::vector<const char*> m_instanceExtensionNames = {};
        std::vector<const char*> m_deviceExtensionNames = {};

        VkPhysicalDeviceFeatures m_enableGpuFeatures = {};           // vulkan 1.0
        VkPhysicalDeviceVulkan11Features m_enable11GpuFeatures = {}; // vulkan 1.1
        VkPhysicalDeviceVulkan12Features m_enable12GpuFeatures = {}; // vulkan 1.2
        VkPhysicalDeviceVulkan13Features m_enable13GpuFeatures = {}; // vulkan 1.3

        VkPhysicalDeviceProperties m_physicalDeviceProperties = {};

    private:
        GLFWwindow* m_window; 
        VulkanSwapchain m_swapchain = {};
        VulkanDevice    m_device    = {};
        VkSurfaceKHR    m_surface   = VK_NULL_HANDLE;
        VulkanInstance  m_instance  = {};

        VkCommandPool m_graphicsCommandPool = VK_NULL_HANDLE;
        VkCommandPool m_computeCommandPool = VK_NULL_HANDLE;
        VkCommandPool m_copyCommandPool = VK_NULL_HANDLE;

        DeletionQueue m_deletionQueue = {};
        VmaAllocator  m_vmaAllocator = {};
        GpuLoaderAsync m_uploader = {};

    private: // cache
        VulkanSamplerCache m_samplerCache = {};
        VulkanShaderCache  m_shaderCache = {};
        VulkanFencePool    m_fencePool = {};

        // descriptor allocator, it don't destroy when swapchain rebuild.
        VulkanDescriptorAllocator m_staticDescriptorAllocator = {};
        VulkanDescriptorLayoutCache m_descriptorLayoutCache = {};

    private: // swapchain relative.
        uint32_t m_imageIndex;
        uint32_t m_currentFrame = 0;

        int32_t  m_maxFramesInFlight;
        std::vector<VkSemaphore> m_semaphoresImageAvailable;
        std::vector<VkSemaphore> m_semaphoresRenderFinished;
        std::vector<VkFence> m_inFlightFences;
        std::vector<VkFence> m_imagesInFlight;
        VkPhysicalDeviceDescriptorIndexingFeaturesEXT m_physicalDeviceDescriptorIndexingFeatures{};
        VkPhysicalDeviceDescriptorIndexingPropertiesEXT m_descriptorIndexingProperties{};

    public:
        DelegatesThreadSafe<RHI> onBeforeSwapchainRebuild;
        DelegatesThreadSafe<RHI> onAfterSwapchainRebuild;
        void forceRebuildSwapchain();

    private:
        RHI() { }
        bool swapchainRebuild();
        void createCommandPool();
        void createSyncObjects();
        void createVmaAllocator();

        void releaseCommandPool();
        void releaseSyncObjects();
        void recreateSwapChain();
        void releaseVmaAllocator();

    public:
        ~RHI();

        static RHI* get();
        void init(
            GLFWwindow* window,
            std::vector<const char*> instanceLayerNames = {},
            std::vector<const char*> instanceExtensionNames = {},
            std::vector<const char*> deviceExtensionNames = {}
        );
        void release();
    
    public:
        uint32_t acquireNextPresentImage();
        void present();
        void waitIdle(){ vkDeviceWaitIdle(m_device); }
        void submitAndResetFence(VkSubmitInfo& info);
        void submitAndResetFence(VulkanSubmitInfo& info);
        void submitAndResetFence(uint32_t count,VkSubmitInfo* infos);
        void submit(VkSubmitInfo& info);
        void submit(VulkanSubmitInfo& info);
        void submit(uint32_t count,VkSubmitInfo* infos);
        void resetFence();

    public: // getter
        VkPhysicalDeviceFeatures& getVkPhysicalDeviceFeatures() { return m_enableGpuFeatures; }
        VmaAllocator& getVmaAllocator() { return m_vmaAllocator; }
        VkQueue& getGraphicsQueue() { return m_device.graphicsQueue; }
        VkQueue& getComputeQueue() { return m_device.computeQueue; }
        const VkDevice& getDevice() const { return m_device.device; }
        VulkanDevice* getVulkanDevice() { return &m_device; }
        VkCommandPool& getGraphicsCommandPool() { return m_graphicsCommandPool; }
        VkCommandPool& getComputeCommandPool() { return m_computeCommandPool; }
        VkCommandPool& getCopyCommandPool() { return m_copyCommandPool; }
        VkInstance getInstance() { return m_instance; }
        VulkanFencePool& getFencePool() { return m_fencePool; }
        VkPhysicalDeviceDescriptorIndexingPropertiesEXT getPhysicalDeviceDescriptorIndexingProperties() const { return m_descriptorIndexingProperties; }

        // for static descriptor cache in RHI.
        VulkanDescriptorLayoutCache& getDescriptorLayoutCache() { return m_descriptorLayoutCache; }

        const uint32_t getCurrentFrameIndex() { return m_currentFrame; }
        VkSemaphore* getCurrentFrameWaitSemaphore() { return &m_semaphoresImageAvailable[m_currentFrame]; }
        VkSemaphore getCurrentFrameWaitSemaphoreRef() { return m_semaphoresImageAvailable[m_currentFrame]; }
        VkSemaphore* getCurrentFrameFinishSemaphore() { return &m_semaphoresRenderFinished[m_currentFrame]; }
        VulkanSwapchain& getSwapchain() { return m_swapchain; }
        std::vector<VkImageView>& getSwapchainImageViews() { return m_swapchain.getImageViews(); }
        std::vector<VkImage>& getSwapchainImages() { return m_swapchain.getImages(); }
        VkFormat getSwapchainFormat() const { return m_swapchain.getSwapchainImageFormat();  }
        VkExtent2D getSwapchainExtent() const { return m_swapchain.getSwapchainExtent();  }
        VkPhysicalDeviceProperties getPhysicalDeviceProperties() const { return m_physicalDeviceProperties; }

        template <typename T> size_t getUniformBufferPadSize() const{ return packUniformBufferOffsetAlignment(sizeof(T)); }

        VkFormat findDepthStencilFormat();

        VulkanShaderCache& getShaderCache() { return m_shaderCache; }

    public:
        VulkanVertexBuffer* createVertexBuffer(const std::vector<float>& data,std::vector<EVertexAttribute>&& as) = delete;
        VulkanVertexBuffer* createVertexBuffer(const std::vector<float>& data,const std::vector<EVertexAttribute>& as);
        VulkanIndexBuffer* createIndexBuffer(const std::vector<uint32_t>& data);
        
        VulkanShaderModule* getShader(const std::string& path,bool bReload = false);
        VkShaderModule getVkShader(const std::string& path,bool bReload = false);
        void addShaderModule(const std::string& path,bool bReload = false);
        
        VkQueue getAsyncCopyQueue() const;
        
        VulkanCommandBuffer* createGraphicsCommandBuffer(VkCommandBufferLevel level = VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        VulkanCommandBuffer* createComputeCommandBuffer(VkCommandBufferLevel level = VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        VulkanCommandBuffer* createCopyCommandBuffer(VkCommandBufferLevel level = VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        
        VulkanDescriptorFactory vkDescriptorFactoryBegin();

        VkRenderPass createRenderpass(const VkRenderPassCreateInfo& info);
        VkPipelineLayout createPipelineLayout(const VkPipelineLayoutCreateInfo& info);
        
        
        void destroyRenderpass(VkRenderPass pass);
        void destroyFramebuffer(VkFramebuffer fb);
        void destroyPipeline(VkPipeline pipe);
        void destroyPipelineLayout(VkPipelineLayout layout);
        
        size_t packUniformBufferOffsetAlignment(size_t originalSize) const;

        GpuLoaderAsync* getUploader() { return &m_uploader; }
        
    public:
        VkSampler getSampler(VkSamplerCreateInfo info, uint32_t& outBindlessIndex);

        VkSampler getPointClampBorderSampler(VkBorderColor);
        VkSampler getPointClampEdgeSampler();
        VkSampler getPointRepeatSampler();
        VkSampler getLinearClampSampler();
        VkSampler getLinearClampNoMipSampler();
        VkSampler getLinearRepeatSampler();

        VkSampler getShadowFilterSampler();

        BindlessSampler* getBindlessSampler() { return m_samplerCache.getBindless(); }
    };

    constexpr int32_t MAX_FRAMES_IN_FLIGHT = 3;
    extern uint32_t getBackBufferCount();
}