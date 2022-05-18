#include "Sampler.h"
#include "RHI.h"
#include "Bindless.h"

#include <sstream>
#include <string>
#include <functional>
#include <crcpp/crc.h>

namespace flower
{
    void VulkanSamplerCache::init(VulkanDevice* newDevice)
    {
	    m_device = newDevice;

        // init bindless set.
        m_bindlessSamplerSet = std::make_unique<BindlessSampler>();
        m_bindlessSamplerSet->init();
    }

    void VulkanSamplerCache::cleanup()
    {
        for(auto& pair : m_samplerCache)
        {
            // release sampler.
            vkDestroySampler(*m_device, pair.second.second, nullptr);
        }
        m_samplerCache.clear();

        // release bindless set.
        m_bindlessSamplerSet->release();
    }

    VkSampler VulkanSamplerCache::createSampler(VkSamplerCreateInfo* info, uint32_t& outBindlessIndex)
    {
        SamplerCreateInfo sci{};
        sci.info = *info;
    
        auto it = m_samplerCache.find(sci);
        if(it != m_samplerCache.end())
        {
            outBindlessIndex = (*it).second.first;
            return (*it).second.second;
        }
        else
        {
            VkSampler sampler;
            vkCheck(vkCreateSampler(*m_device, &sci.info, nullptr, &sampler));

            uint32_t bindlessIndex = m_bindlessSamplerSet->updateSamplerToBindlessDescriptorSet(sampler);
            outBindlessIndex = bindlessIndex;

            m_samplerCache[sci] = { bindlessIndex, sampler };
            return sampler;
        }
    }

    bool VulkanSamplerCache::SamplerCreateInfo::operator==(const SamplerCreateInfo& other) const
    {
        return  (other.info.addressModeU == this->info.addressModeU) && 
                (other.info.addressModeV == this->info.addressModeV) && 
                (other.info.addressModeW == this->info.addressModeW) && 
                (other.info.anisotropyEnable == this->info.anisotropyEnable) && 
                (other.info.borderColor == this->info.borderColor) && 
                (other.info.compareEnable == this->info.compareEnable) && 
                (other.info.compareOp == this->info.compareOp) && 
                (other.info.flags == this->info.flags) && 
                (other.info.magFilter == this->info.magFilter) && 
                (other.info.maxAnisotropy == this->info.maxAnisotropy) && 
                (other.info.maxLod == this->info.maxLod) && 
                (other.info.minFilter == this->info.minFilter) && 
                (other.info.minLod == this->info.minLod) && 
                (other.info.mipmapMode == this->info.mipmapMode) && 
                (other.info.unnormalizedCoordinates == this->info.unnormalizedCoordinates);
    }

    size_t VulkanSamplerCache::SamplerCreateInfo::hash() const
    {
        return CRC::Calculate(&this->info, sizeof(VkSamplerCreateInfo), CRC::CRC_32());
    }

    void SamplerFactory::buildPointClampBorderSampler(VkBorderColor borderColor)
    {
        this->
             MagFilter(VK_FILTER_NEAREST)
            .MinFilter(VK_FILTER_NEAREST)
            .MipmapMode(VK_SAMPLER_MIPMAP_MODE_NEAREST)
            .AddressModeU(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER)
            .AddressModeV(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER)
            .AddressModeW(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER)
            .BorderColor(borderColor)
            .UnnormalizedCoordinates(VK_FALSE)
            .MaxAnisotropy(1.0f)
            .AnisotropyEnable(VK_FALSE)
            .MinLod(-10000.0f)
            .MaxLod(10000.0f)
            .MipLodBias(0.0f);
    }

    void SamplerFactory::buildPointClampEdgeSampler()
    {
        this->
             MagFilter(VK_FILTER_NEAREST)
            .MinFilter(VK_FILTER_NEAREST)
            .MipmapMode(VK_SAMPLER_MIPMAP_MODE_NEAREST)
            .AddressModeU(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
            .AddressModeV(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
            .AddressModeW(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
            .CompareEnable(VK_FALSE)
            .UnnormalizedCoordinates(VK_FALSE)
            .MaxAnisotropy(1.0f)
            .AnisotropyEnable(VK_FALSE)
            .MinLod(-10000.0f)
            .MaxLod(10000.0f)
            .MipLodBias(0.0f);
    }

    void SamplerFactory::buildPointRepeatSampler()
    {
        this->
             MagFilter(VK_FILTER_NEAREST)
            .MinFilter(VK_FILTER_NEAREST)
            .MipmapMode(VK_SAMPLER_MIPMAP_MODE_NEAREST)
            .AddressModeU(VK_SAMPLER_ADDRESS_MODE_REPEAT)
            .AddressModeV(VK_SAMPLER_ADDRESS_MODE_REPEAT)
            .AddressModeW(VK_SAMPLER_ADDRESS_MODE_REPEAT)
            .CompareOp(VK_COMPARE_OP_LESS)
            .CompareEnable(VK_FALSE)
            .BorderColor(VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK)
            .UnnormalizedCoordinates(VK_FALSE)
            .MaxAnisotropy(1.0f)
            .AnisotropyEnable(VK_FALSE)
            .MinLod(-10000.0f)
            .MaxLod(10000.0f)
            .MipLodBias(0.0f);
    }

    void SamplerFactory::buildLinearClampSampler()
    {
        this->
             MagFilter(VK_FILTER_LINEAR)
            .MinFilter(VK_FILTER_LINEAR)
            .MipmapMode(VK_SAMPLER_MIPMAP_MODE_LINEAR)
            .AddressModeU(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
            .AddressModeV(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
            .AddressModeW(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
            .CompareOp(VK_COMPARE_OP_LESS)
            .CompareEnable(VK_FALSE)
            .BorderColor(VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE)
            .UnnormalizedCoordinates(VK_FALSE)
            .MaxAnisotropy(1.0f)
            .AnisotropyEnable(VK_FALSE)
            .MinLod(-10000.0f)
            .MaxLod(10000.0f)
            .MipLodBias(0.0f);
    }

    void SamplerFactory::buildLinearClampNoMipSampler()
    {
        this->
             MagFilter(VK_FILTER_LINEAR)
            .MinFilter(VK_FILTER_LINEAR)
            .MipmapMode(VK_SAMPLER_MIPMAP_MODE_NEAREST)
            .AddressModeU(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
            .AddressModeV(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
            .AddressModeW(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
            .CompareOp(VK_COMPARE_OP_LESS)
            .CompareEnable(VK_FALSE)
            .BorderColor(VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE)
            .UnnormalizedCoordinates(VK_FALSE)
            .MaxAnisotropy(1.0f)
            .AnisotropyEnable(VK_FALSE)
            .MinLod(-10000.0f)
            .MaxLod(10000.0f)
            .MipLodBias(0.0f);
    }

    void SamplerFactory::buildLinearRepeatSampler()
    {
        this->
             MagFilter(VK_FILTER_LINEAR)
            .MinFilter(VK_FILTER_LINEAR)
            .MipmapMode(VK_SAMPLER_MIPMAP_MODE_LINEAR)
            .AddressModeU(VK_SAMPLER_ADDRESS_MODE_REPEAT)
            .AddressModeV(VK_SAMPLER_ADDRESS_MODE_REPEAT)
            .AddressModeW(VK_SAMPLER_ADDRESS_MODE_REPEAT)
            .CompareOp(VK_COMPARE_OP_LESS)
            .CompareEnable(VK_FALSE)
            .BorderColor(VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK)
            .UnnormalizedCoordinates(VK_FALSE)
            .MaxAnisotropy(1.0f)
            .AnisotropyEnable(VK_FALSE)
            .MinLod(-10000.0f)
            .MaxLod(10000.0f)
            .MipLodBias(0.0f);
    }

    void SamplerFactory::buildShadowFilterSampler()
    {
        this->
             MagFilter(VK_FILTER_NEAREST)
            .MinFilter(VK_FILTER_NEAREST)
            .MipmapMode(VK_SAMPLER_MIPMAP_MODE_NEAREST)
            .AddressModeU(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER)
            .AddressModeV(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER)
            .AddressModeW(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER)
            .CompareOp(VK_COMPARE_OP_GREATER)
            .CompareEnable(VK_TRUE)
            .BorderColor(VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK)
            .UnnormalizedCoordinates(VK_FALSE)
            .MaxAnisotropy(1.0f)
            .AnisotropyEnable(VK_FALSE)
            .MinLod(-10000.0f)
            .MaxLod(10000.0f)
            .MipLodBias(0.0f);
    }

}