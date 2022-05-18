#pragma once
#include "Common.h"
#include "Buffer.h"
#include "Fence.h"
#include "Image.h"
#include <queue>

namespace flower
{
    struct CombineTextureBindless;
    struct TextureInfo;
    // gpu loader 
    class GpuLoaderAsync
    {
    private:
        // prepare stage buffer.
        VulkanBuffer* m_stageBuffer = nullptr;

        VkCommandPool   m_pool = VK_NULL_HANDLE;
        VkCommandBuffer m_cmdBuf = VK_NULL_HANDLE;
        VulkanFence*    m_fence = nullptr;
    public:
        struct TextureLoadTask
        {
            CombineTextureBindless* combineTexture;
            Texture2DImage* createImage;
            TextureInfo* textureInfo;
        };

        static const size_t MAX_UPLOAD_SIZE = 128 * 1024 * 1024;

    private:
        bool m_bFirstTick = true;
        std::queue<TextureLoadTask> m_textureLoadTasks;

        std::vector<TextureLoadTask> m_uploadingTaskes;
        bool m_bProcessing = false;

        std::atomic<bool> m_notifyClose = false;

        std::condition_variable m_wakeCondition;
        std::mutex m_wakeMutex;

        void asyncTick();

    public:
        void init();

        void blockFlush();
        void release();

        void addTextureLoadTask(const TextureLoadTask& in);
    };
}