#pragma once
#include "../RHI/RHI.h"
#include <set>

namespace flower
{
	struct RenderTextureCreateInfo 
	{
		VkImageType imageType = VK_IMAGE_TYPE_2D;
		VkFormat format;
		VmaMemoryUsage memUsage = VMA_MEMORY_USAGE_GPU_ONLY;

		VkExtent3D extent = { 1, 1, 1};
		uint32_t mipLevels = 1;
		uint32_t arrayLayers = 1;
		VkImageUsageFlags usage;
		VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
		VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
	};

	// simple render target pool, if temporal render texture mark release.
	// add to free pool, use for next require pass if hash match.
	class RenderTexturePool
	{
	public:
		class RenderTextureReference
		{
			friend RenderTexturePool;
		private:
			RenderTexturePool* m_pool = nullptr;
			VulkanImage* m_image      = nullptr;

			uint32_t m_descHash;
			size_t m_physicalId;

		public:
			void release();

			bool isValid() const;
			VulkanImage* getImage();
		};
		
	private:
		std::unordered_map<uint32_t, std::vector<VulkanImage*>> m_allocatedImages = {};
		std::unordered_map<uint32_t, std::set<size_t>> m_freePools = {};

	public:
		// request texture reference.
		// use release to free texture.
		// all image view should call requestImageView() to get new one.
		RenderTextureReference requestRenderTexture(const RenderTextureCreateInfo& info);

		void release();
	};

	using RenderTextureRef = RenderTexturePool::RenderTextureReference;
}