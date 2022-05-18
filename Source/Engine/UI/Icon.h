#pragma once
#include <vulkan/vulkan.h>
#include <string>
#include <memory>
#include <unordered_map>
#include <map>

namespace flower
{
    struct AssetMeta;
    struct TextureInfo;

    class Texture2DImage;

    class IconInfo
    {
    public:
        IconInfo(){}
        IconInfo(const std::string& path, bool flip = true, VkSampler sampler = VK_NULL_HANDLE);

        IconInfo(const std::string& path, AssetMeta* inMeta, TextureInfo* inInfo);
        ~IconInfo();

        Texture2DImage* getIcon() { return m_icon; }
        void* getId();

    private:
        void* m_cacheId = nullptr;
        Texture2DImage* m_icon = nullptr;
        VkSampler m_sampler = VK_NULL_HANDLE;
    };

    class IconLibrary
    {
    private:
        // never unload.
        // cache icon for editor.
        std::unordered_map<std::string/*icon name*/,std::unique_ptr<IconInfo>/*cache*/> m_cacheIconInfo;

        // never unload.
        // cache snap shots for editor.
        std::map<std::string, std::unique_ptr<IconInfo>/*cache*/> m_cacheSnapShots;

    public:
        void init();
        void release();

        IconInfo* getIconInfo(const std::string& name);

        void releaseSnapShot(const std::string& path);
        void loadSnapShotFromTextureMetaFileLDR(const std::string& path, AssetMeta* inMeta, TextureInfo* inInfo);

        IconInfo* getSnapShots(const std::string& name);
    };
}