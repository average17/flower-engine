#include "Device.h"
#include "RHI.h"
#include <set>
#include <mutex>

namespace flower
{
	static PFN_vkSetDebugUtilsObjectNameEXT gVkSetDebugUtilsObjectName = nullptr;
	static PFN_vkCmdBeginDebugUtilsLabelEXT gVkCmdBeginDebugUtilsLabel = nullptr;
	static PFN_vkCmdEndDebugUtilsLabelEXT   gVkCmdEndDebugUtilsLabel = nullptr;

	void extDebugUtilsGetProcAddresses(VkDevice device)
	{
		gVkSetDebugUtilsObjectName = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT");
		gVkCmdBeginDebugUtilsLabel = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetDeviceProcAddr(device, "vkCmdBeginDebugUtilsLabelEXT");
		gVkCmdEndDebugUtilsLabel = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetDeviceProcAddr(device, "vkCmdEndDebugUtilsLabelEXT");
	}

	void VulkanDevice::init(
		VkInstance instance,
		VkSurfaceKHR surface,
		VkPhysicalDeviceFeatures features,
		const std::vector<const char*>& device_request_extens,
		void* nextChain)
	{
		this->m_instance = instance;
		this->m_surface = surface;
		this->m_deviceCreateNextChain = nextChain;

		// 0. pick best gpu
		pickupSuitableGpu(instance,device_request_extens);

		// store gpu memory properties.
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		// 1. create logic device
		this->m_deviceExtensions = device_request_extens;
		this->m_openFeatures = features;
		createLogicDevice();

		m_cacheSupportDepthOnlyFormat = findDepthOnlyFormat();
		m_cacheSupportDepthStencilFormat = findDepthStencilFormat();

		extDebugUtilsGetProcAddresses(device);
	}

	void VulkanDevice::destroy()
	{
		if (device != VK_NULL_HANDLE)
		{
			vkDestroyDevice(device, nullptr);
		}
	}

	void VulkanDevice::createLogicDevice()
	{
		auto indices = findQueueFamilies();

		graphicsFamily = indices.graphicsFamily;
		computeFamily  = indices.computeFaimly;
		copyFamily = indices.transferFamily;

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

		// First we ensure present queue in which family.
		bool bPresentInGraphics = indices.presentFamily == indices.graphicsFamily;
		bool bPresentInCompute  = indices.presentFamily == indices.computeFaimly;
		bool bPresentInTransfer = indices.presentFamily == indices.transferFamily;
		CHECK(bPresentInCompute || bPresentInGraphics || bPresentInTransfer);

		// any optimize? async queue use 0.5f priority?
		// all queue priority is 1.0f.
		std::vector<float> graphicsQueuePriority(indices.graphicsQueueCount, 1.0f);
		std::vector<float>   computeQueuePriority(indices.computeQueueCount, 1.0f);
		std::vector<float> transferQueuePriority(indices.transferQueueCount, 0.5f); // 0.5 to avoid graphics resource race.
		{ // major graphics queue.

			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = indices.graphicsFamily;
			queueCreateInfo.queueCount = indices.graphicsQueueCount;
			queueCreateInfo.pQueuePriorities = graphicsQueuePriority.data();
			queueCreateInfos.push_back(queueCreateInfo);
		}

		{ // major compute queue
			if(indices.bSoloComputeQueueFamily)   // only request when queue is solo.
			{
				VkDeviceQueueCreateInfo queueCreateInfo{};
				queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueCreateInfo.queueFamilyIndex = indices.computeFaimly;
				queueCreateInfo.queueCount = indices.computeQueueCount;
				queueCreateInfo.pQueuePriorities = computeQueuePriority.data();
				queueCreateInfos.push_back(queueCreateInfo);
			}
		}

		{ // major transfer queue.
			if(indices.bSoloTransferFamily)   // only request when queue solo.
			{
				VkDeviceQueueCreateInfo queueCreateInfo{};
				queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueCreateInfo.queueFamilyIndex = indices.transferFamily;
				queueCreateInfo.queueCount = indices.transferQueueCount;
				queueCreateInfo.pQueuePriorities = transferQueuePriority.data();
				queueCreateInfos.push_back(queueCreateInfo);
			}
		}

		VkDeviceCreateInfo createInfo{};
		VkPhysicalDeviceFeatures2 physicalDeviceFeatures2{};

		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		
		// we use physical device features2.
		createInfo.pEnabledFeatures = nullptr;
		createInfo.pNext = &physicalDeviceFeatures2;
		{
			physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
			physicalDeviceFeatures2.features = m_openFeatures;
			physicalDeviceFeatures2.pNext = m_deviceCreateNextChain;
		}

		createInfo.enabledExtensionCount = static_cast<uint32_t>(m_deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = m_deviceExtensions.data();

		// no special device layer, all control by instance layer.
		createInfo.ppEnabledLayerNames = NULL;
		createInfo.enabledLayerCount = 0;

		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) 
		{
			LOG_GRAPHICS_FATAL("Create vulkan logic device.");
		}
		// get major queues.
		vkGetDeviceQueue(device,indices.graphicsFamily,0, &graphicsQueue);
		vkGetDeviceQueue(device,indices.computeFaimly, 0, &computeQueue);

		if(bPresentInGraphics)
		{
			if(indices.graphicsQueueCount == 1)
			{
				vkGetDeviceQueue(device,indices.graphicsFamily, 0, &presentQueue);
			}
			else if(indices.graphicsQueueCount == 2)
			{
				vkGetDeviceQueue(device,indices.graphicsFamily, 1, &presentQueue);
			}
			else if(indices.graphicsQueueCount > 2)
			{
				vkGetDeviceQueue(device,indices.graphicsFamily, 1, &presentQueue);
				std::vector<VkQueue> tmpQueues (indices.graphicsQueueCount - 2);
				for(size_t index = 0; index < tmpQueues.size(); index++)
				{
					tmpQueues[index] = VK_NULL_HANDLE;
					vkGetDeviceQueue(device, indices.graphicsFamily, (uint32_t)index + 2, &tmpQueues[index]);
				}
				asyncGraphicsQueues.swap(tmpQueues);
			}
		}
		else
		{
			vkGetDeviceQueue(device, indices.presentFamily, 0, &presentQueue);

			if(indices.graphicsQueueCount > 1)
			{
				std::vector<VkQueue> tmpQueues (indices.graphicsQueueCount - 1);
				for(size_t index = 0; index<tmpQueues.size(); index++)
				{
					tmpQueues[index] = VK_NULL_HANDLE;
					vkGetDeviceQueue(device,indices.graphicsFamily,(uint32_t)index + 1,&tmpQueues[index]);
				}
				asyncGraphicsQueues.swap(tmpQueues);
			}
		}

		if(indices.computeQueueCount > 0 && indices.bSoloComputeQueueFamily)
		{
			std::vector<VkQueue> tmpQueues (indices.computeQueueCount);
			for(size_t index = 0; index < tmpQueues.size(); index++)
			{
				tmpQueues[index] = VK_NULL_HANDLE;
				vkGetDeviceQueue(device,indices.computeFaimly,(uint32_t)index, &tmpQueues[index]);
			}
			asyncComputeQueues.swap(tmpQueues);
		}

		if(indices.transferQueueCount > 0 && indices.bSoloTransferFamily)
		{
			std::vector<VkQueue> tmpQueues (indices.transferQueueCount);
			for(size_t index = 0; index < tmpQueues.size(); index++)
			{
				tmpQueues[index] = VK_NULL_HANDLE;
				vkGetDeviceQueue(device, indices.transferFamily,(uint32_t)index, &tmpQueues[index]);
			}
			asyncTransferQueues.swap(tmpQueues);
		}
	}

	VkFormat VulkanDevice::findSupportedFormat(
		const std::vector<VkFormat>& candidates,
		VkImageTiling tiling,
		VkFormatFeatureFlags features)
	{
		for (VkFormat format : candidates) 
		{
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) 
			{
				return format;
			} 
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) 
			{
				return format;
			}
		}

		LOG_GRAPHICS_FATAL("Can't find supported format.");
	}

	VkFormat VulkanDevice::findDepthStencilFormat()
	{
		return findSupportedFormat
		(
			{ VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}

	VkFormat VulkanDevice::findDepthOnlyFormat()
	{
		return findSupportedFormat
		(
			{ VK_FORMAT_D32_SFLOAT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}

	void VulkanDevice::pickupSuitableGpu(VkInstance& instance,const std::vector<const char*>& device_request_extens)
	{
		uint32_t physical_device_count { 0 };
		vkCheck(vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr));
		if (physical_device_count < 1)
		{
			LOG_GRAPHICS_FATAL("No gpu support vulkan on your computer.");
		}
		std::vector<VkPhysicalDevice> physical_devices;
		physical_devices.resize(physical_device_count);
		vkCheck(vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices.data()));

		ASSERT(!physical_devices.empty(),"No gpu on your computer.");

		for (auto &gpu : physical_devices)
		{
			VkPhysicalDeviceProperties device_properties;
			vkGetPhysicalDeviceProperties(gpu, &device_properties);
			if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				this->physicalDevice = gpu;
				if (isPhysicalDeviceSuitable(device_request_extens)) 
				{
					LOG_GRAPHICS_INFO("Using discrete gpu: {0}.",vkToString(device_properties.deviceName));
					physicalDeviceProperties = device_properties;
					return;
				}
			}
		}

		LOG_GRAPHICS_WARN("No discrete gpu found, using default gpu.");

		this->physicalDevice = physical_devices.at(0);
		if (isPhysicalDeviceSuitable(device_request_extens)) 
		{
			this->physicalDevice = physical_devices.at(0);
			VkPhysicalDeviceProperties device_properties;
			vkGetPhysicalDeviceProperties(this->physicalDevice,&device_properties);
			LOG_GRAPHICS_INFO("Choose default gpu: {0}.",vkToString(device_properties.deviceName));
			physicalDeviceProperties = device_properties;
			return;
		}
		else
		{
			LOG_GRAPHICS_FATAL("No suitable gpu can use.");
		}
	}

	bool VulkanDevice::isPhysicalDeviceSuitable(const std::vector<const char*>& device_request_extens)
	{
		VulkanQueueFamilyIndices indices = findQueueFamilies();

		bool extensionsSupported = checkDeviceExtensionSupport(device_request_extens);

		bool swapChainAdequate = false;
		if (extensionsSupported) 
		{
			auto swapChainSupport = querySwapchainSupportDetail();
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		return indices.isCompleted() && extensionsSupported && swapChainAdequate;
	}

	VulkanQueueFamilyIndices VulkanDevice::findQueueFamilies()
	{
		VulkanQueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(this->physicalDevice, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(this->physicalDevice, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto& queueFamily : queueFamilies) 
		{
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.graphicsFamily = i;
				indices.bGraphicsFamilySet = true;
				indices.graphicsQueueCount = queueFamily.queueCount; 
			}
			else if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) 
			{
				indices.computeFaimly = i;
				indices.bComputeFaimlySet = true;

				// is solo compute queue.
				indices.bSoloComputeQueueFamily = true;
				indices.computeQueueCount = queueFamily.queueCount;
			}
			else if(queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
			{
				indices.transferFamily = i;
				indices.bTransferFamilySet = true;

				// is solo transfer queue.
				indices.bSoloTransferFamily = true; 
				indices.transferQueueCount = queueFamily.queueCount;
			}

			// use first support present family. commonly it is graphics family.
			if (!indices.bPresentFamilySet)
			{
				VkBool32 presentSupport = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, m_surface, &presentSupport);
				if (presentSupport)
				{
					indices.presentFamily = i;
					indices.bPresentFamilySet = true;
				}
			}

			if (indices.isCompleted()) 
			{
				break;
			}
			i++;
		}

		// fallback to graphics queue if no solo queue family.
		if(!indices.isCompleted())
		{
			CHECK(indices.bGraphicsFamilySet);

			if(!indices.bComputeFaimlySet)
			{
				indices.computeFaimly = indices.graphicsFamily;
				indices.bComputeFaimlySet = true;
				indices.computeQueueCount = 1;
			}

			if(!indices.bTransferFamilySet)
			{
				indices.transferFamily = indices.graphicsFamily;
				indices.bTransferFamilySet = true;
			}
		}

		if (indices.graphicsFamily != indices.presentFamily)
		{
			LOG_ERROR("Graphics family no same as presemt family. maybe cause graphics tears problem.");
		}

		return indices;
	}

	VulkanSwapchainSupportDetails VulkanDevice::querySwapchainSupportDetail()
	{
		VulkanSwapchainSupportDetails details;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_surface, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice,m_surface, &formatCount, nullptr);
		if (formatCount != 0) 
		{
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_surface, &presentModeCount, nullptr);
		if (presentModeCount != 0) 
		{
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	uint32_t VulkanDevice::findMemoryType(uint32_t typeFilter,VkMemoryPropertyFlags properties)
	{
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) 
		{
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) 
			{
				return i;
			}
		}

		LOG_GRAPHICS_FATAL("No suitable memory type can find.");
	}

	void VulkanDevice::printAllQueueFamiliesInfo()
	{
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

		for(auto &q_family : queueFamilies)
		{
			LOG_GRAPHICS_INFO("queue id: {}." ,  vkToString(q_family.queueCount));
			LOG_GRAPHICS_INFO("queue flag: {}.", vkToString(q_family.queueFlags));
		}
	}

	bool VulkanDevice::checkDeviceExtensionSupport(const std::vector<const char*>& device_request_extens)
	{
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(device_request_extens.begin(), device_request_extens.end());

		for (const auto& extension : availableExtensions) 
		{
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	void setResourceName(VkObjectType objectType,uint64_t handle,const char* name)
	{
		VkDevice device = RHI::get()->getDevice();

		static std::mutex gMutexForSetResource;
		if (gVkSetDebugUtilsObjectName && handle && name)
		{
			std::unique_lock<std::mutex> lock(gMutexForSetResource);

			VkDebugUtilsObjectNameInfoEXT nameInfo = {};
			nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
			nameInfo.objectType = objectType;
			nameInfo.objectHandle = handle;
			nameInfo.pObjectName = name;

			gVkSetDebugUtilsObjectName(device, &nameInfo);
		}
	}

	void setPerfMarkerBegin(VkCommandBuffer cmd_buf, const char* name,const glm::vec4& color)
	{
		if (gVkCmdBeginDebugUtilsLabel)
		{
			VkDebugUtilsLabelEXT label = {};
			label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
			label.pLabelName = name;
			label.color[0] = color.r;
			label.color[1] = color.g;
			label.color[2] = color.b;
			label.color[3] = color.a;
			gVkCmdBeginDebugUtilsLabel( cmd_buf, &label );
		}
	}

	void setPerfMarkerEnd(VkCommandBuffer cmd_buf)
	{
		if (gVkCmdEndDebugUtilsLabel)
		{
			gVkCmdEndDebugUtilsLabel(cmd_buf);
		}
	}

}