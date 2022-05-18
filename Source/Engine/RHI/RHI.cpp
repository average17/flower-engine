#include "RHI.h"
#include "../Core/Core.h"

namespace flower
{
    uint32_t getBackBufferCount()
    {
        return (uint32_t)MAX_FRAMES_IN_FLIGHT;
    }

	RHI* RHI::get()
	{
        // thread safe since C++ 11.
		static RHI rhi {};
		return &rhi;
	}

    RHI::~RHI()
    {
        release();
    }

	void RHI::init(
		GLFWwindow* window,
		std::vector<const char*> instanceLayerNames,
		std::vector<const char*> instanceExtensionNames,
		std::vector<const char*> deviceExtensionNames)
	{
        if(m_bOk) return;

        m_maxFramesInFlight = MAX_FRAMES_IN_FLIGHT;

        m_instanceExtensionNames = instanceExtensionNames;
        m_instanceExtensionNames.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        m_instanceExtensionNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        m_instanceLayerNames = instanceLayerNames;

        m_deviceExtensionNames = deviceExtensionNames;
        m_deviceExtensionNames.push_back(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);
        m_deviceExtensionNames.push_back(VK_KHR_MAINTENANCE1_EXTENSION_NAME);
        m_deviceExtensionNames.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
        m_deviceExtensionNames.push_back(VK_KHR_MAINTENANCE3_EXTENSION_NAME);

        // Enable gpu features 1.0 here.
        m_enableGpuFeatures.samplerAnisotropy = true; 
        m_enableGpuFeatures.depthClamp = true;        
        m_enableGpuFeatures.shaderSampledImageArrayDynamicIndexing = true;
        m_enableGpuFeatures.multiDrawIndirect = VK_TRUE; 
        m_enableGpuFeatures.drawIndirectFirstInstance = VK_TRUE;
        m_enableGpuFeatures.independentBlend = VK_TRUE;
        m_enableGpuFeatures.multiViewport = VK_TRUE;
        m_enableGpuFeatures.fragmentStoresAndAtomics = VK_TRUE;

        // Enable gpu features 1.1 here.
        m_enable11GpuFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
        m_enable11GpuFeatures.pNext = &m_enable12GpuFeatures;
        m_enable11GpuFeatures.shaderDrawParameters = VK_TRUE;

        // Enable gpu features 1.2 here.
        m_enable12GpuFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        m_enable12GpuFeatures.drawIndirectCount = VK_TRUE;
        m_enable12GpuFeatures.drawIndirectCount = VK_TRUE;
        m_enable12GpuFeatures.imagelessFramebuffer = VK_TRUE;
        m_enable12GpuFeatures.separateDepthStencilLayouts = VK_TRUE;
        m_enable12GpuFeatures.descriptorIndexing = VK_TRUE;
        m_enable12GpuFeatures.runtimeDescriptorArray = VK_TRUE;
        m_enable12GpuFeatures.descriptorBindingPartiallyBound = VK_TRUE;
        m_enable12GpuFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE;
        m_enable12GpuFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
        m_enable12GpuFeatures.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
        m_enable12GpuFeatures.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
        m_enable12GpuFeatures.timelineSemaphore = VK_TRUE;
        m_enable12GpuFeatures.pNext = nullptr; // todo: vulkan 1.3

        // Enable gpu features 1.3 here.
        m_enable13GpuFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        m_enable13GpuFeatures.pNext = nullptr; // todo: vulkan 1.4
        m_enable13GpuFeatures.dynamicRendering = VK_TRUE;

        m_window = window;
        m_instance.init(m_instanceExtensionNames,m_instanceLayerNames);
        m_deletionQueue.push([&]()
        {
            m_instance.destroy();
        });

        if (glfwCreateWindowSurface(m_instance, window, nullptr, &m_surface) != VK_SUCCESS) 
        {
            LOG_GRAPHICS_FATAL("Window surface create error.");
        }
        m_deletionQueue.push([&]()
        {
            if(m_surface!=VK_NULL_HANDLE)
            {
                vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
            }
        });

        m_deviceExtensionNames.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        m_device.init(m_instance,m_surface,m_enableGpuFeatures,m_deviceExtensionNames, &m_enable11GpuFeatures);
        vkGetPhysicalDeviceProperties(m_device.physicalDevice, &m_physicalDeviceProperties);
        LOG_GRAPHICS_INFO("Gpu min align memory size: {0}.",m_physicalDeviceProperties.limits.minUniformBufferOffsetAlignment);

        auto vkGetPhysicalDeviceProperties2KHR = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2KHR>(vkGetInstanceProcAddr(m_instance,"vkGetPhysicalDeviceProperties2KHR"));
        CHECK(vkGetPhysicalDeviceProperties2KHR);
        VkPhysicalDeviceProperties2KHR deviceProperties{};
        m_descriptorIndexingProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES_EXT;
        deviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
        deviceProperties.pNext = &m_descriptorIndexingProperties;
        vkGetPhysicalDeviceProperties2KHR(m_device.physicalDevice,&deviceProperties);

        m_swapchain.init(&m_device,&m_surface,window);
        createCommandPool();
        createSyncObjects();
        m_deletionQueue.push([&]()
        {
            releaseSyncObjects();
            releaseCommandPool();
            m_swapchain.destroy();
            m_device.destroy();
        });

        // Custom vulkan object cache and allocator.
        m_samplerCache.init(&m_device);
        m_shaderCache.init(m_device.device);
        m_fencePool.init(m_device.device);
        m_descriptorLayoutCache.init(&m_device);
        m_staticDescriptorAllocator.init(&m_device);
        createVmaAllocator();
        m_deletionQueue.push([&]()
        {
            releaseVmaAllocator();
            m_staticDescriptorAllocator.cleanup();
            m_descriptorLayoutCache.cleanup();
            m_shaderCache.release(); 
            m_samplerCache.cleanup();
            m_fencePool.release();
        });

        m_uploader.init();
        m_deletionQueue.push([&]()
        {
            m_uploader.release();
        });

        m_bOk = true;
	}

    void RHI::release()
    {
        if(!m_bOk) return;
        m_deletionQueue.flush();
        m_bOk = false;
    }

    VkFormat RHI::findDepthStencilFormat()
    {
        return m_device.findDepthStencilFormat();
    }

    VulkanVertexBuffer* RHI::createVertexBuffer(const std::vector<float>& data,const std::vector<EVertexAttribute>& as)
    { 
        return VulkanVertexBuffer::create(&m_device,m_graphicsCommandPool,data,as); 
    }

    VulkanIndexBuffer* RHI::createIndexBuffer(const std::vector<uint32_t>& data)
    {
        return VulkanIndexBuffer::create(&m_device,m_graphicsCommandPool,data);
    }

    VulkanShaderModule* RHI::getShader(const std::string& path,bool bReload)
    {
        return m_shaderCache.getShader(path,bReload);
    }

    VkShaderModule RHI::getVkShader(const std::string& path,bool bReload)
    {
        return m_shaderCache.getShader(path,bReload)->GetModule();
    }

    void RHI::addShaderModule(const std::string& path,bool bReload)
    {
        m_shaderCache.getShader(path,bReload);
    }

    VkQueue RHI::getAsyncCopyQueue() const
    {
        if(m_device.asyncTransferQueues.size() > 1)
        {
            return m_device.asyncTransferQueues[1];
        }
        else if(m_device.asyncTransferQueues.size()==1)
        {
            return m_device.asyncTransferQueues[0];
        }
        else if( m_device.asyncGraphicsQueues.size() > 0)
        {
            return m_device.asyncGraphicsQueues[m_device.asyncGraphicsQueues.size() - 1];
        }
        else
        {
            LOG_WARN("No async copy queue can use.");
            return m_device.presentQueue;
        }
    }

    bool RHI::swapchainRebuild()
    {
        static int current_width;
        static int current_height;
        glfwGetWindowSize(m_window,&current_width,&current_height);

        static int last_width = current_width;
        static int last_height = current_height;

        if(current_width != last_width || current_height != last_height)
        {
            last_width = current_width;
            last_height = current_height;
            return true;
        }

        return false;
    }

    void RHI::createCommandPool()
    {
        auto queueFamilyIndices = m_device.findQueueFamilies();

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_graphicsCommandPool) != VK_SUCCESS) 
        {
            LOG_GRAPHICS_FATAL("Fail to create vulkan CommandPool.");
        }

        poolInfo.queueFamilyIndex =  queueFamilyIndices.computeFaimly;
        if(vkCreateCommandPool(m_device,&poolInfo,nullptr,&m_computeCommandPool)!=VK_SUCCESS)
        {
            LOG_GRAPHICS_FATAL("Fail to create compute CommandPool.");
        }

        VkCommandPoolCreateInfo copyPoolInfo{};
        copyPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

        // todo: check if there exist solo transfer queue.
        copyPoolInfo.queueFamilyIndex = queueFamilyIndices.transferFamily;
        copyPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        if(vkCreateCommandPool(m_device,&poolInfo,nullptr,&m_copyCommandPool)!=VK_SUCCESS)
        {
            LOG_GRAPHICS_FATAL("Fail to create copy CommandPool.");
        }
    }

    VulkanDescriptorFactory RHI::vkDescriptorFactoryBegin()
    {
        return VulkanDescriptorFactory::begin(&m_descriptorLayoutCache,&m_staticDescriptorAllocator);
    }

    VkRenderPass RHI::createRenderpass(const VkRenderPassCreateInfo& info)
    {
        VkRenderPass pass;
        vkCheck(vkCreateRenderPass(m_device, &info, nullptr, &pass));
        return pass;
    }

    VkPipelineLayout RHI::createPipelineLayout(const VkPipelineLayoutCreateInfo& info)
    {
        VkPipelineLayout layout;
        vkCheck(vkCreatePipelineLayout(m_device,&info,nullptr,&layout));
        return layout;
    }

    void RHI::destroyRenderpass(VkRenderPass pass)
    {
        vkDestroyRenderPass(m_device,pass,nullptr);
    }

    void RHI::destroyFramebuffer(VkFramebuffer fb)
    {
        vkDestroyFramebuffer(m_device,fb,nullptr);
    }

    void RHI::destroyPipeline(VkPipeline pipe)
    {
        vkDestroyPipeline(m_device,pipe,nullptr);
    }

    void RHI::destroyPipelineLayout(VkPipelineLayout layout)
    {
        vkDestroyPipelineLayout(m_device,layout,nullptr);
    }

    size_t RHI::packUniformBufferOffsetAlignment(size_t originalSize) const
    {
        size_t minUboAlignment = m_physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
        size_t alignedSize = originalSize;
        if(minUboAlignment > 0)
        {
            alignedSize = (alignedSize+minUboAlignment-1) &~ (minUboAlignment-1);
        }
        return alignedSize;
    }

    VkSampler RHI::getSampler(VkSamplerCreateInfo info, uint32_t& outBindlessIndex)
    {
        return m_samplerCache.createSampler(&info, outBindlessIndex);
    }

    VkSampler RHI::getPointClampBorderSampler(VkBorderColor borderColor)
    {
        SamplerFactory sfa{};
        sfa.buildPointClampBorderSampler(borderColor);

        uint32_t index = 0;
        return RHI::get()->getSampler(sfa.getCreateInfo(), index);
    }

    VkSampler RHI::getPointClampEdgeSampler()
    {
        SamplerFactory sfa{};
        sfa.buildPointClampEdgeSampler();

        uint32_t index = 0;
        return RHI::get()->getSampler(sfa.getCreateInfo(), index);
    }

    VkSampler RHI::getPointRepeatSampler()
    {
        SamplerFactory sfa{};
        sfa.buildPointRepeatSampler();
            
        uint32_t index = 0;
        return RHI::get()->getSampler(sfa.getCreateInfo(), index);
    }

    VkSampler RHI::getLinearClampSampler()
    {
        SamplerFactory sfa{};
        sfa.buildLinearClampSampler();

        uint32_t index = 0;
        return RHI::get()->getSampler(sfa.getCreateInfo(), index);
    }

    VkSampler RHI::getLinearClampNoMipSampler()
    {
        SamplerFactory sfa{};
        sfa.buildLinearClampNoMipSampler();

        uint32_t index = 0;
        return RHI::get()->getSampler(sfa.getCreateInfo(), index);
    }

    VkSampler RHI::getLinearRepeatSampler()
    {
        SamplerFactory sfa{};
        sfa.buildLinearRepeatSampler();

        uint32_t index = 0;
        return RHI::get()->getSampler(sfa.getCreateInfo(), index);
    }

    VkSampler RHI::getShadowFilterSampler()
    {
        SamplerFactory sfa{};
        sfa.buildShadowFilterSampler();

        uint32_t index = 0;
        return RHI::get()->getSampler(sfa.getCreateInfo(), index);
    }

    void RHI::createSyncObjects()
    {
        const auto imageNums = m_swapchain.getImageViews().size();
        m_semaphoresImageAvailable.resize(imageNums);
        m_semaphoresRenderFinished.resize(imageNums);
        m_inFlightFences.resize(imageNums);
        m_imagesInFlight.resize(imageNums);
        for(auto& fence : m_imagesInFlight)
        {
            fence = VK_NULL_HANDLE;
        }

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for(size_t i = 0; i < imageNums; i++)
        {
            if(
                vkCreateSemaphore(m_device,&semaphoreInfo,nullptr,&m_semaphoresImageAvailable[i])!=VK_SUCCESS||
                vkCreateSemaphore(m_device,&semaphoreInfo,nullptr,&m_semaphoresRenderFinished[i])!=VK_SUCCESS||
                vkCreateFence(m_device,&fenceInfo,nullptr,&m_inFlightFences[i])!=VK_SUCCESS)
            {
                LOG_GRAPHICS_FATAL("Fail to create semaphore.");
            }
        }
    }

    void RHI::createVmaAllocator()
    {
        if(CVarSystem::get()->getInt32CVar("r.RHI.EnableVma"))
        {
            VmaAllocatorCreateInfo allocatorInfo = {};
            allocatorInfo.physicalDevice = m_device.physicalDevice;
            allocatorInfo.device = m_device;
            allocatorInfo.instance = m_instance;
            vmaCreateAllocator(&allocatorInfo, &m_vmaAllocator);
        }
    }

    void RHI::releaseVmaAllocator()
    {
        if(CVarSystem::get()->getInt32CVar("r.RHI.EnableVma"))
        {
            vmaDestroyAllocator(m_vmaAllocator);
        }
    }

    VulkanCommandBuffer* RHI::createGraphicsCommandBuffer(VkCommandBufferLevel level)
    {
        return VulkanCommandBuffer::create(
            &m_device,
            m_graphicsCommandPool,
            level,
            m_device.graphicsQueue
        );
    }

    VulkanCommandBuffer* RHI::createComputeCommandBuffer(VkCommandBufferLevel level)
    {
        return VulkanCommandBuffer::create(
            &m_device,
            m_computeCommandPool,
            level,
            m_device.computeQueue
        );
    }

    VulkanCommandBuffer* RHI::createCopyCommandBuffer(VkCommandBufferLevel level)
    {
        VkQueue asyncQueue = getAsyncCopyQueue();

        return VulkanCommandBuffer::create(
            &m_device,
            m_copyCommandPool,
            level,
            asyncQueue != VK_NULL_HANDLE ? asyncQueue : m_device.presentQueue
        );
    }

    void RHI::releaseCommandPool()
    {
        if(m_graphicsCommandPool!=VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(m_device, m_graphicsCommandPool, nullptr);
        }
        if(m_computeCommandPool!=VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(m_device, m_computeCommandPool, nullptr);
        }
        if(m_copyCommandPool!=VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(m_device, m_copyCommandPool, nullptr);
        }
    }

    void RHI::releaseSyncObjects()
    {
        for(size_t i = 0; i < m_semaphoresImageAvailable.size(); i++)
        {
            vkDestroySemaphore(m_device,m_semaphoresImageAvailable[i],nullptr);
            vkDestroySemaphore(m_device,m_semaphoresRenderFinished[i],nullptr);
            vkDestroyFence(m_device,m_inFlightFences[i],nullptr);
        }
    }

    void RHI::recreateSwapChain()
    {
        vkDeviceWaitIdle(m_device);

        static int width = 0, height = 0;
        glfwGetFramebufferSize(m_window, &width, &height);

        // just return if swapchain width or height is 0.
        if(width == 0 || height == 0)
        {
            m_swapchainChange = true;
            return;
        }

        // Clean special
        onBeforeSwapchainRebuild.broadcast();

        releaseSyncObjects();
        m_swapchain.destroy();
        m_swapchain.init(&m_device,&m_surface,m_window);
        createSyncObjects();

        m_imagesInFlight.resize(m_swapchain.getImageViews().size(), VK_NULL_HANDLE);

        // Recreate special
        onAfterSwapchainRebuild.broadcast();
    }
	
	void RHI::forceRebuildSwapchain()
	{
		m_swapchainChange = true;
	}

    uint32_t RHI::acquireNextPresentImage()
    {
        m_swapchainChange |= swapchainRebuild();

        vkWaitForFences(m_device,1,&m_inFlightFences[m_currentFrame],VK_TRUE,UINT64_MAX);
        VkResult result = vkAcquireNextImageKHR(
            m_device,m_swapchain,UINT64_MAX,
            m_semaphoresImageAvailable[m_currentFrame],
            VK_NULL_HANDLE,&m_imageIndex
        );

        if(result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            recreateSwapChain();
        }
        else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            LOG_GRAPHICS_FATAL("Fail to requeset present image.");
        }

        if(m_imagesInFlight[m_imageIndex] != VK_NULL_HANDLE)
        {
            vkWaitForFences(m_device,1,&m_imagesInFlight[m_imageIndex],VK_TRUE,UINT64_MAX);
        }

        m_imagesInFlight[m_imageIndex] = m_inFlightFences[m_currentFrame];

        return m_imageIndex;
    }

    void RHI::present()
    {
        VkPresentInfoKHR present_info{};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        VkSemaphore signalSemaphores[] = { m_semaphoresRenderFinished[m_currentFrame] };
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapchains[] = { m_swapchain };
        present_info.swapchainCount = 1;
        present_info.pSwapchains = swapchains;
        present_info.pImageIndices = &m_imageIndex;

        auto result = vkQueuePresentKHR(m_device.presentQueue,&present_info);
        if(result==VK_ERROR_OUT_OF_DATE_KHR || result==VK_SUBOPTIMAL_KHR || m_swapchainChange)
        {
            m_swapchainChange = false;
            recreateSwapChain();
        }
        else if(result != VK_SUCCESS)
        {
            LOG_GRAPHICS_FATAL("Fail to present image.");
        }

        // if swapchain rebuild and on minimized, still add frame.
        m_currentFrame = (m_currentFrame + 1) % m_maxFramesInFlight;
    }

    void RHI::submit(VkSubmitInfo& info)
    {
        vkCheck(vkQueueSubmit(getGraphicsQueue(),1, &info, m_inFlightFences[m_currentFrame]));
    }

    void RHI::submit(VulkanSubmitInfo& info)
    {
        vkCheck(vkQueueSubmit(getGraphicsQueue(),1,&info.get(),m_inFlightFences[m_currentFrame]));
    }

    void RHI::submit(uint32_t count,VkSubmitInfo* infos)
    {
        vkCheck(vkQueueSubmit(getGraphicsQueue(),count,infos,m_inFlightFences[m_currentFrame]));
    }

    void RHI::resetFence()
    {
        vkCheck(vkResetFences(m_device,1,&m_inFlightFences[m_currentFrame]));
    }

    void RHI::submitAndResetFence(VkSubmitInfo& info)
    {
        vkCheck(vkResetFences(m_device,1,&m_inFlightFences[m_currentFrame]));
        vkCheck(vkQueueSubmit(getGraphicsQueue(),1,&info,m_inFlightFences[m_currentFrame]));
    }

    void RHI::submitAndResetFence(VulkanSubmitInfo& info)
    {
        vkCheck(vkResetFences(m_device,1,&m_inFlightFences[m_currentFrame]));
        vkCheck(vkQueueSubmit(getGraphicsQueue(),1,&info.get(),m_inFlightFences[m_currentFrame]));
    }

    void RHI::submitAndResetFence(uint32_t count,VkSubmitInfo* infos)
    {
        vkCheck(vkResetFences(m_device,1,&m_inFlightFences[m_currentFrame]));
        vkCheck(vkQueueSubmit(getGraphicsQueue(),count,infos,m_inFlightFences[m_currentFrame]));
    }
}


