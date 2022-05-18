#pragma once

#include "Device.h"
#include "Error.h"
#include <array>
#include <Vma/vk_mem_alloc.h>

namespace flower
{
    class VulkanImage
    {
    protected:
        VulkanDevice* m_device = nullptr;

        VkImage m_image         = VK_NULL_HANDLE;
        VkDeviceMemory m_memory = VK_NULL_HANDLE;
        VmaAllocation m_allocation = nullptr;

        VkImageView m_imageView = VK_NULL_HANDLE;

        VkDeviceSize  m_size = {};
        VkImageLayout m_currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VkImageCreateInfo m_createInfo = {};

        std::unordered_map<uint32_t, VkImageView> m_cacheImageViews{};

    public:
        VulkanImage() = default;

        virtual void create(
            VulkanDevice* device,
            const VkImageCreateInfo& info, 
            VkImageViewType viewType,
            VkImageAspectFlags aspectMask,
            bool isHost,
            VmaMemoryUsage memUsage = VMA_MEMORY_USAGE_GPU_ONLY,
            bool bCreateView = true
        ); 

        // use for render texture pool.
        static VulkanImage* create(
            const VkImageCreateInfo& info, 
            VkImageViewType viewType,
            VkImageAspectFlags aspectMask,
            bool isHost,
            VmaMemoryUsage memUsage = VMA_MEMORY_USAGE_GPU_ONLY
        );

        VkImageView requestImageView(VkImageViewCreateInfo viewInfo);
        static uint32_t getImageViewHash(VkImageViewCreateInfo viewInfo);

        void generateMipmaps(VkCommandBuffer cb,int32_t texWidth,int32_t texHeight,uint32_t mipLevels,VkImageAspectFlagBits flag);
        void copyBufferToImage(VkCommandBuffer cb,VkBuffer buffer,uint32_t width,uint32_t height,uint32_t depth,VkImageAspectFlagBits flag,uint32_t mipmapLevel = 0);
        void setCurrentLayout(VkImageLayout oldLayout) { m_currentLayout = oldLayout; }
        void transitionLayout(VkCommandBuffer cb,VkImageLayout newLayout,VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);
   
    public:
        void transitionLayoutImmediately(VkCommandPool pool,VkQueue queue,VkImageLayout newLayout,VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);
        virtual ~VulkanImage(){ release(); }
        VkImage getImage() const { return m_image; }
        VkImageView getImageView() const { return m_imageView; }
        VkDeviceMemory getMem() const { return m_memory; }
        VkFormat getFormat() const { return m_createInfo.format; }
        VkExtent3D getExtent() const { return m_createInfo.extent; }
        const VkImageCreateInfo& getInfo() const { return m_createInfo; }
        VkImageLayout getCurentLayout() const { return m_currentLayout; }

        void clear(VkCommandBuffer cb, glm::vec4 colour = {0, 0, 0, 0});
        void clear(VkCommandBuffer cb, glm::uvec4 colour);
        void upload(std::vector<uint8_t>& bytes,VkCommandPool pool,VkQueue queue,VkImageAspectFlagBits flag = VK_IMAGE_ASPECT_COLOR_BIT);
        void release();
    };

    class Texture2DImage: public VulkanImage
    {
        Texture2DImage() = default;
    public:
        virtual ~Texture2DImage() = default;

        static Texture2DImage* create(
            VulkanDevice* device,
            uint32_t width, uint32_t height,
            VkImageAspectFlags aspectMask,
            VkFormat format,
            bool isHost = false,
            int32_t mipLevels = -1 // -1 meaning use full miplevels.
        );

        static Texture2DImage* createAndUpload(
            VulkanDevice* device,
            VkCommandPool pool,VkQueue queue,
            std::vector<uint8_t>& bytes,
            uint32_t width,uint32_t height,
            VkImageAspectFlags aspectMask,
            VkFormat format,
            VkImageAspectFlagBits flag = VK_IMAGE_ASPECT_COLOR_BIT,
            bool isHost = false,
            int32_t mipLevels = -1 // -1 meaning use full miplevels.
        );

        float getMipmapLevels() const
        {
            return (float)m_createInfo.mipLevels;
        }
    };

    class DepthStencilImage: public VulkanImage
    {
        DepthStencilImage() = default;

        // prepare depth only image view for shader read.
        VkImageView m_depthOnlyImageView = VK_NULL_HANDLE;

    public:
        virtual ~DepthStencilImage();
        VkImageView getDepthOnlyImageView() const { return m_depthOnlyImageView; }

        static DepthStencilImage* create(
            VulkanDevice* device,
            uint32_t width, 
            uint32_t height,
            bool bCreateDepthOnlyView);
    };

    class DepthOnlyImage: public VulkanImage
    {
        DepthOnlyImage() = default;

    public:
        virtual ~DepthOnlyImage();

        static DepthOnlyImage* create(VulkanDevice* device,uint32_t width, uint32_t height);
    };

    class VulkanImageArray: public VulkanImage
    {
    private:
        bool m_bInitArrayViews = false;

    protected:
        // per-layer view.
        std::vector<VkImageView> m_perTextureView{};
        void release();

    public:
        VkImageView getArrayImageView(uint32_t layerIndex) const { return m_perTextureView[layerIndex]; }
        const std::vector<VkImageView>& getArrayImageViews() { return m_perTextureView; }

        void initViews(const std::vector<VkImageViewCreateInfo> infos);

        virtual ~VulkanImageArray()
        {
            release();
        }
    };
    
    class DepthOnlyTextureArray : public VulkanImageArray
    {
        DepthOnlyTextureArray() = default;
        
    public:
        static DepthOnlyTextureArray* create(VulkanDevice*device,uint32_t width,uint32_t height,uint32_t layerCount);
    };

    class RenderTexture : public VulkanImage
    {
        RenderTexture() = default;
        bool m_bInitMipmapViews = false;
        std::vector<VkImageView> m_perMipmapTextureView{};

        void release();
    public:
        virtual ~RenderTexture()
        {
            release();
        }

        static RenderTexture* create(VulkanDevice* device,uint32_t width,uint32_t height,VkFormat format,VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT);
        static RenderTexture* create(VulkanDevice* device,uint32_t width,uint32_t height, uint32_t depth,VkFormat format,VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT);
        static RenderTexture* create(VulkanDevice* device,uint32_t width,uint32_t height,int32_t mipmapCount,bool bCreateMipmaps,VkFormat format,VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT);
       
        VkImageView getMipmapView(uint32_t level);
    };

    class RenderTextureCube : public VulkanImage
    {
        RenderTextureCube() = default;
        bool m_bInitMipmapViews = false;
		std::vector<VkImageView> m_perMipmapTextureView{};

		void release();
		
    public:
        virtual ~RenderTextureCube() 
        {
            release();
        }
        static RenderTextureCube* create(
            VulkanDevice*device,
            uint32_t width,
            uint32_t height,
            int32_t mipmapLevels,
            VkFormat format,
            VkImageUsageFlags usage = 
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | 
                VK_IMAGE_USAGE_SAMPLED_BIT,
            bool bCreateViewsForMips = false
         );

        VkImageView getMipmapView(uint32_t level);
    };

}