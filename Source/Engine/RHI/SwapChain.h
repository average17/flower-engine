#pragma once
#include "Device.h"
#include <glfw/glfw3.h>

namespace flower
{
	class VulkanSwapchain
	{
	private:
		VulkanDevice* m_device  = nullptr;
		VkSurfaceKHR* m_surface = nullptr;
		GLFWwindow*   m_window  = nullptr;
		std::vector<VkImage> m_swapchainImages = {};
		std::vector<VkImageView> m_swapchainImageViews = {};
		VkFormat m_swapchainImageFormat = {};
		VkExtent2D m_swapchainExtent = {};
		VkSwapchainKHR m_swapchain = {};
		bool m_bOk = false;

	private:
		void createSwapchain();
		VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats); 
		VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D chooseSwapchainExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	public:
		VulkanSwapchain() = default;
		~VulkanSwapchain();
		operator VkSwapchainKHR()
		{
			return m_swapchain;
		}

		void init(VulkanDevice* in_device,VkSurfaceKHR* in_surface,GLFWwindow* in_window);
		void destroy();
		const VkExtent2D& getSwapchainExtent() const { return m_swapchainExtent; }
		auto& getImages() { return m_swapchainImages; }
		VulkanDevice* getDevice() { return m_device; }
		const VkFormat& getSwapchainImageFormat() const { return m_swapchainImageFormat;}
		std::vector<VkImageView>& getImageViews() { return m_swapchainImageViews; }
		VkSwapchainKHR& getInstance(){ return m_swapchain;}

		void rebuildSwapchain();
	};
}