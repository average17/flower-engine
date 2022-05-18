#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <mutex>
#include "../Core/Core.h"

namespace flower
{
	class VulkanQueueFamilyIndices 
	{
		friend class VulkanDevice;

	public:
		uint32_t graphicsFamily; 
		uint32_t graphicsQueueCount;

		uint32_t presentFamily;  
		uint32_t presentQueueCount;

		uint32_t computeFaimly;  
		uint32_t computeQueueCount;

		uint32_t transferFamily;
		uint32_t transferQueueCount;

		bool isCompleted() 
		{
			return bGraphicsFamilySet && bPresentFamilySet && bComputeFaimlySet && bTransferFamilySet;
		}

	private:
		bool bGraphicsFamilySet = false;
		bool bPresentFamilySet = false;
		bool bComputeFaimlySet = false;
		bool bTransferFamilySet = false;

		// solo meaning the queue family is standalone.
		bool bSoloComputeQueueFamily = false;
		bool bSoloTransferFamily = false;
	};

	struct VulkanSwapchainSupportDetails 
	{
		VkSurfaceCapabilitiesKHR        capabilities; 
		std::vector<VkSurfaceFormatKHR> formats; 
		std::vector<VkPresentModeKHR>   presentModes;
	};

	class VulkanDevice
	{
	public:
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VkDevice device                 = VK_NULL_HANDLE;
		VkPhysicalDeviceProperties      physicalDeviceProperties = {};

		VkQueue graphicsQueue = VK_NULL_HANDLE; // Major graphics queue.
		VkQueue presentQueue  = VK_NULL_HANDLE; // Major present queue.
		VkQueue computeQueue  = VK_NULL_HANDLE; // Major compute queue.

		uint32_t graphicsFamily;
		uint32_t copyFamily;
		uint32_t computeFamily;

		// other queue unused, can use for async transfer purpose.
		std::vector<VkQueue> asyncTransferQueues;    // all useful transfer queues.
		std::vector<VkQueue> asyncComputeQueues;     // all useful compute queues.
		std::vector<VkQueue> asyncGraphicsQueues;    // all useful graphics queues.

		VkPhysicalDeviceMemoryProperties memProperties;

	private:
		VkInstance   m_instance = VK_NULL_HANDLE;
		VkSurfaceKHR m_surface  = VK_NULL_HANDLE;

		std::vector<const char*> m_deviceExtensions = {};
		VkPhysicalDeviceFeatures m_openFeatures = {};
		void* m_deviceCreateNextChain = nullptr;

		VkFormat m_cacheSupportDepthStencilFormat;
		VkFormat m_cacheSupportDepthOnlyFormat;
	public:
		operator VkDevice() { return device; }
		VulkanDevice() = default;
		~VulkanDevice(){ }

		void init(
			VkInstance instance,
			VkSurfaceKHR surface,
			VkPhysicalDeviceFeatures features = {},
			const std::vector<const char*>& device_request_extens= {},
			void* nextChain = nullptr
		);

		void destroy();
		VulkanQueueFamilyIndices findQueueFamilies();
		uint32_t findMemoryType(uint32_t typeFilter,VkMemoryPropertyFlags properties);
		VulkanSwapchainSupportDetails querySwapchainSupportDetail();

		void printAllQueueFamiliesInfo();
		const auto& getDeviceExtensions() const { return m_deviceExtensions; }
		const auto& getDeviceFeatures()   const { return m_openFeatures; }

		VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates,VkImageTiling tiling,VkFormatFeatureFlags features);
		VkFormat findDepthStencilFormat();
		VkFormat findDepthOnlyFormat();

		VkFormat getDepthOnlyFormat() const { return m_cacheSupportDepthOnlyFormat; }
		VkFormat getDepthStencilFormat() const { return m_cacheSupportDepthStencilFormat;}

	private:
		void pickupSuitableGpu(VkInstance& instance,const std::vector<const char*>& device_request_extens);
		bool isPhysicalDeviceSuitable(const std::vector<const char*>& device_request_extens);
		bool checkDeviceExtensionSupport(const std::vector<const char*>& device_request_extens);
		void createLogicDevice();
	};

	extern void setResourceName(VkObjectType objectType, uint64_t handle, const char* name);
	extern void setPerfMarkerBegin(VkCommandBuffer cmd_buf, const char* name,const glm::vec4& color);
	extern void setPerfMarkerEnd(VkCommandBuffer cmd_buf);
}