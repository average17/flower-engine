#include "RenderTexturePool.h"
#include <crcpp/crc.h>

namespace flower
{
	void RenderTexturePool::release()
	{
		RHI::get()->waitIdle();

		for(auto& pair : m_allocatedImages)
		{
			for(VulkanImage* image : pair.second)
			{
				delete image;
				image =  nullptr;
			}
		}

		m_allocatedImages.clear();
		m_freePools.clear();
	}

	RenderTexturePool::RenderTextureReference RenderTexturePool::requestRenderTexture(const RenderTextureCreateInfo& info)
	{
		RenderTextureReference result{};

		RenderTextureCreateInfo ci = info;
		uint32_t descHash = CRC::Calculate(&ci, sizeof(RenderTextureCreateInfo), CRC::CRC_32());

		result.m_pool = this;
		result.m_descHash = descHash;

		auto& imagePool = m_allocatedImages[descHash];
		auto& freeHashSet = m_freePools[descHash];

		// if free hash set exist free elements, just use that.
		if(freeHashSet.size() > 0)
		{
			result.m_physicalId = *freeHashSet.end();
			result.m_image = imagePool[result.m_physicalId];

			// remove free elemt.
			freeHashSet.erase(freeHashSet.end());
		}
		else
		{
			VkImageCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			createInfo.flags = {};
			
			createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;

			createInfo.tiling = info.tiling;
			createInfo.usage = info.usage;
			createInfo.samples = info.samples;
			createInfo.imageType = info.imageType;
			createInfo.format = info.format;
			createInfo.extent = info.extent;
			createInfo.mipLevels = info.mipLevels;
			createInfo.arrayLayers = info.arrayLayers;

			// we need to create new one.
			VulkanImage* newImage = VulkanImage::create(
				createInfo,
				VK_IMAGE_VIEW_TYPE_MAX_ENUM, // don't create image view so just fill max enum.
				VK_IMAGE_ASPECT_NONE_KHR, // don't create image view so just fill none.
				false, // don't create image view.
				info.memUsage
			);

			result.m_image = newImage;
			result.m_physicalId = imagePool.size();

			imagePool.push_back(newImage);
		}

		return result;
	}

	void RenderTexturePool::RenderTextureReference::release()
	{
		auto& freeHashSet = m_pool->m_freePools[m_descHash];
		CHECK((!freeHashSet.contains(m_physicalId)) && 
			"Some error happen, free pool should no contains data before we release.");
		
		freeHashSet.emplace(m_physicalId);
	}

	VulkanImage* RenderTexturePool::RenderTextureReference::getImage()
	{
		return m_image;
	}

	bool RenderTexturePool::RenderTextureReference::isValid() const
	{ 
		return m_pool != nullptr && m_image != nullptr;
	}
}