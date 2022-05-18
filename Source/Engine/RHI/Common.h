#pragma once
#include <string>
#include <vulkan/vulkan.h>
#include <sstream>
#include "../Core/Core.h"

namespace flower
{
	inline uint32_t getMipLevelsCount(uint32_t texWidth,uint32_t texHeight)
	{
		return static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
	}

	inline void executeImmediately(
		VkDevice device,
		VkCommandPool commandPool,
		VkQueue queue,
		const std::function<void(VkCommandBuffer cb)>& func)
	{
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(commandBuffer, &beginInfo);
		func(commandBuffer);

		vkEndCommandBuffer(commandBuffer);
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;
		vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(queue);
		vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
	}

	enum class EVertexAttribute : uint32_t
	{
		none = 0,
		pos,	        // vec3
		uv0,            // vec2
		uv1,	        // vec2
		normal,         // vec3
		tangent,        // vec4
		color,          // vec3
		alpha,          // float
		count,
	};

	inline int32_t getVertexAttributeSize(EVertexAttribute va)
	{
		switch(va)
		{
		case EVertexAttribute::alpha:
			return sizeof(float);
			break;

		case EVertexAttribute::uv0:
		case EVertexAttribute::uv1:
			return 2 * sizeof(float);
			break;

		case EVertexAttribute::pos:
		case EVertexAttribute::color:
		case EVertexAttribute::normal:
			return 3 * sizeof(float);
			break;

		case EVertexAttribute::tangent:
			return 4 * sizeof(float);
			break;

		case EVertexAttribute::none:
		case EVertexAttribute::count:
		default:
			LOG_GRAPHICS_ERROR("Unknown vertex_attribute type.");
			return 0;
			break;
		}
		return 0;
	}

	inline int32_t getVertexAttributeElementCount(EVertexAttribute va)
	{
		return getVertexAttributeSize(va) / sizeof(float);
	}

	inline VkFormat toVkFormat(EVertexAttribute va)
	{
		VkFormat format = VK_FORMAT_R32G32B32_SFLOAT;
		switch(va)
		{
		case EVertexAttribute::alpha:
			format = VK_FORMAT_R32_SFLOAT;
			break;

		case EVertexAttribute::uv0:
		case EVertexAttribute::uv1:
			format = VK_FORMAT_R32G32_SFLOAT;
			break;

		case EVertexAttribute::pos:
		case EVertexAttribute::normal:
		case EVertexAttribute::color:
			format = VK_FORMAT_R32G32B32_SFLOAT;
			break;

		case EVertexAttribute::tangent:
			format = VK_FORMAT_R32G32B32A32_SFLOAT;
			break;

		case EVertexAttribute::none:
		case EVertexAttribute::count:
		default:
			LOG_GRAPHICS_FATAL("Unkown vertex_attribute type.");
			break;
		}
		return format;
	}

}