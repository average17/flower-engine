#include "Icon.h"
#include "../RHI/RHI.h"

#include "../Asset/AssetTexture.h"
#include "../Asset/AssetCommon.h"
#include "../Asset/AssetSystem.h"

#include <ImGui/ImGuiVulkan.h>

namespace flower
{
    IconInfo::IconInfo(const std::string& path, bool flip, VkSampler sampler)
    {
        m_icon = loadTextureFromFile(path, VK_FORMAT_R8G8B8A8_SRGB, 4,flip,true);
        m_sampler = sampler;

        if(m_sampler == VK_NULL_HANDLE)
        {
            m_sampler = RHI::get()->getLinearClampSampler();
        }
    }

    IconInfo::IconInfo(const std::string& path,AssetMeta* inMeta,TextureInfo* inInfo)
    {
        std::vector<uint8_t> pixelData{};
        pixelData.resize(inInfo->metaBinDataSize);
        uncompressTextureMetaBin(
            inInfo, 
            inMeta->binBlob.data(), 
            inMeta->binBlob.size(), 
            (char*)pixelData.data()
        );

        m_icon = Texture2DImage::createAndUpload(
            RHI::get()->getVulkanDevice(),
            RHI::get()->getGraphicsCommandPool(),
            RHI::get()->getGraphicsQueue(),
            pixelData,
            SNAPSHOT_SIZE,
            SNAPSHOT_SIZE,
            VK_IMAGE_ASPECT_COLOR_BIT,
            toVkFormat(inInfo->format)
        );

        m_sampler = RHI::get()->getLinearClampSampler();
    }

    IconInfo::~IconInfo()
    {
        delete m_icon;
    }

    void* IconInfo::getId()
    {
        if(m_cacheId != nullptr)
        {
            return m_cacheId;
        }
        else
        {
            m_cacheId = (void*)ImGui_ImplVulkan_AddTexture(m_sampler,m_icon->getImageView(), m_icon->getCurentLayout());
            return m_cacheId;
        }
    }

    void IconLibrary::init()
    {
        m_cacheIconInfo.clear();
    }

    void IconLibrary::release()
    {
        m_cacheIconInfo.clear();
        m_cacheSnapShots.clear();
    }

    IconInfo* IconLibrary::getIconInfo(const std::string& name)
    {
        if(!m_cacheIconInfo[name] || m_cacheIconInfo[name]->getIcon() == nullptr)
        {
            m_cacheIconInfo[name] = std::make_unique<IconInfo>(name);
        }

        return m_cacheIconInfo[name].get();
    }

    void IconLibrary::releaseSnapShot(const std::string& path)
    {
        // flush device when delete snap shot.
        RHI::get()->waitIdle();
        if(m_cacheSnapShots.contains(path))
        {
            if(m_cacheSnapShots[path] != nullptr)
            {
                m_cacheSnapShots[path].reset();
            }
            m_cacheSnapShots.erase(path);
        }
    }

    void IconLibrary::loadSnapShotFromTextureMetaFileLDR(const std::string& path,AssetMeta* inMeta,TextureInfo* inInfo)
    {
        // when reload, should release first then load manually.
        CHECK(!m_cacheSnapShots.contains(path));
        {
            m_cacheSnapShots[path] = std::make_unique<IconInfo>(path, inMeta, inInfo);
        }
    }

    IconInfo* IconLibrary::getSnapShots(const std::string& name)
    {
        if(m_cacheSnapShots.contains(name))
        {
            return m_cacheSnapShots[name].get();
        }

        return nullptr;
    }


}