#pragma once

#include "Device.h"
#include <unordered_map>

namespace flower
{
    struct VulkanDescriptorLayoutReference
    {
        VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    };

    struct VulkanDescriptorSetReference
    {
        VkDescriptorSet set = VK_NULL_HANDLE;
    };

    class VulkanDescriptorAllocator
    {
        friend class VulkanDescriptorFactory;
    public:
        struct PoolSizes
        {
            std::vector<std::pair<VkDescriptorType,float>> sizes =
            {
                { VK_DESCRIPTOR_TYPE_SAMPLER,                .5f },
                { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.f },
                { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          4.f },
                { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          1.f },
                { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,   1.f },
                { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,   1.f },
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         2.f },
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         2.f },
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.f },
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.f },
                { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,       .5f }
            };
        };

    private:
        VulkanDevice* m_device;
        VkDescriptorPool m_currentPool = VK_NULL_HANDLE;
        PoolSizes m_descriptorSizes;
        std::vector<VkDescriptorPool> m_usedPools;
        std::vector<VkDescriptorPool> m_freePools;

        VkDescriptorPool requestPool();
    public:
        // reset all using pool to free.
        void resetPools();

        // allocate descriptor, maybe fail.
        [[nodiscard]]bool allocate(VkDescriptorSet* set,VkDescriptorSetLayout layout);

        void init(VulkanDevice* newDevice);
        void cleanup();
    };

    class VulkanDescriptorLayoutCache
    {
    public:
        struct DescriptorLayoutInfo
        {
            std::vector<VkDescriptorSetLayoutBinding> bindings;
            bool operator==(const DescriptorLayoutInfo& other) const;
            size_t hash() const;
        };

    private:
        VulkanDevice* m_device;

        struct DescriptorLayoutHash
        {
            std::size_t operator()(const DescriptorLayoutInfo& k) const
            {
                return k.hash();
            }
        };

        typedef std::unordered_map<DescriptorLayoutInfo,VkDescriptorSetLayout,DescriptorLayoutHash> DescriptorLayoutCache;
        DescriptorLayoutCache m_layoutCache;

    public:
        void init(VulkanDevice* newDevice);
        void cleanup();
        VkDescriptorSetLayout createDescriptorLayout(VkDescriptorSetLayoutCreateInfo* info);
    };

    class VulkanDescriptorFactory
    {
    public:
        // start building.
        static VulkanDescriptorFactory begin(VulkanDescriptorLayoutCache* layoutCache,VulkanDescriptorAllocator* allocator);

        // use for buffer.
        VulkanDescriptorFactory& bindBuffer(uint32_t binding,VkDescriptorBufferInfo* bufferInfo,VkDescriptorType type,VkShaderStageFlags stageFlags);

        // use for texture
        VulkanDescriptorFactory& bindImage(uint32_t binding,VkDescriptorImageInfo*,VkDescriptorType type,VkShaderStageFlags stageFlags);

        // use for texture array.
        VulkanDescriptorFactory& bindImages(uint32_t binding,uint32_t count,VkDescriptorImageInfo*,VkDescriptorType type,VkShaderStageFlags stageFlags);

        bool build(VulkanDescriptorSetReference& set,VulkanDescriptorLayoutReference& layout);
        bool build(VulkanDescriptorSetReference& set);
        bool build(VkDescriptorSet* set);

    private:
        struct DescriptorWriteContainer
        {
            VkDescriptorImageInfo* imgInfo;
            VkDescriptorBufferInfo* bufInfo;
            uint32_t binding;
            VkDescriptorType type;
            uint32_t count;
            bool isImg = false;
        };

        std::vector<DescriptorWriteContainer> m_descriptorWriteBufInfos{ };
        std::vector<VkDescriptorSetLayoutBinding> m_bindings;

        VulkanDescriptorLayoutCache* m_cache;
        VulkanDescriptorAllocator* m_allocator;
    };

}