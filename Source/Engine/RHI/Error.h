#pragma once
#include <vulkan/vulkan.h>

#include "../Core/Core.h"
#include "../Core/DeletionQueue.h"

namespace flower
{
	template <class T>
	inline std::string vkToString(const T &value)
	{
		std::stringstream ss;
		ss << std::fixed << value;
		return ss.str();
	}

	class VulkanException : public std::runtime_error
	{
	public:
		VulkanException(VkResult result,const std::string &msg = "vulkan exception"):
			result {result}, std::runtime_error {msg}
		{
			errorMessage = std::string(std::runtime_error::what())+std::string{" : "} + vkToString(result);
		}

		const char *what() const noexcept override
		{
			return errorMessage.c_str();
		}

		VkResult result;
	private:
		std::string errorMessage;
	};

#ifdef FLOWER_DEBUG
	inline void vkCheck(VkResult err)
	{
		if (err)
		{ 
			LOG_GRAPHICS_FATAL("check error: {}.", vkToString(err));
			__debugbreak(); 
			throw VulkanException(err);
			abort(); 
		} 
	}

	inline void vkHandle(decltype(VK_NULL_HANDLE) handle)
	{
		if (handle == VK_NULL_HANDLE) 
		{ 
			LOG_GRAPHICS_FATAL("Handle is empty.");
			__debugbreak(); 
			abort(); 
		}
	}
#else
	inline void vkCheck(VkResult err)
	{

	}

	inline void vkHandle(decltype(VK_NULL_HANDLE) handle)
	{

	}
#endif // FLOWER_DEBUG
}