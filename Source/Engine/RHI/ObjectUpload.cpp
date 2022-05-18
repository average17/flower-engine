#include "ObjectUpload.h"
#include "RHI.h"
#include "../Asset/AssetCommon.h"
#include "../Asset/AssetSystem.h"
#include "../Engine.h"

namespace flower
{
    static AutoCVarInt32 cVarBaseVRamForUploadBuffer(
        "r.Vram.BaseUploadBuffer",
        "Persistent vram for upload buffer(mb).",
        "Vram",
        128,
        CVarFlags::InitOnce | CVarFlags::ReadOnly
    );

    auto getWholeStagingbufferSize()
    {
        return cVarBaseVRamForUploadBuffer.get() * 1024 * 1024;
    }

    void GpuLoaderAsync::init()
    {
        CHECK(m_stageBuffer == nullptr);
        m_fence = RHI::get()->getFencePool().createFence(false);
    
        std::thread asyncThread([this]()
        {
            VkCommandPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.queueFamilyIndex = RHI::get()->getVulkanDevice()->copyFamily;
            poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            if (vkCreateCommandPool(RHI::get()->getDevice(),&poolInfo,nullptr,&m_pool) != VK_SUCCESS)
            {
                LOG_GRAPHICS_FATAL("Fail to create vulkan CommandPool.");
            }

            VkDeviceSize baseBufferSize = static_cast<VkDeviceSize>(getWholeStagingbufferSize());

            m_stageBuffer = VulkanBuffer::create(
                RHI::get()->getVulkanDevice(),
                m_pool,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                VMA_MEMORY_USAGE_CPU_TO_GPU,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                baseBufferSize,
                nullptr
            );

            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool = m_pool;
            allocInfo.commandBufferCount = 1;
            vkAllocateCommandBuffers(*RHI::get()->getVulkanDevice(),&allocInfo,&m_cmdBuf);

            while(!m_notifyClose.load())
            {
                if(m_textureLoadTasks.size() > 0 || m_bProcessing)
                {
                    asyncTick();
                }
                else
                {
                    std::unique_lock<std::mutex> lock(m_wakeMutex);
                    m_wakeCondition.wait(lock);
                }
            }
            
            delete m_stageBuffer;
            m_stageBuffer = nullptr;
            
            vkFreeCommandBuffers(RHI::get()->getDevice(),m_pool,1,&m_cmdBuf);
            m_cmdBuf = VK_NULL_HANDLE;
            vkDestroyCommandPool(RHI::get()->getDevice(), m_pool, nullptr);
            m_notifyClose.store(false);
        });

        asyncThread.detach();
    }

    void GpuLoaderAsync::blockFlush()
    {
        while(true)
        {
            if(m_textureLoadTasks.size()==0 && !m_bProcessing)
            {
                return;
            }
        }
    }

    void GpuLoaderAsync::release()
    {
        CHECK(m_stageBuffer != nullptr);

        m_notifyClose.store(true);
        m_wakeCondition.notify_one();

        while(m_notifyClose.load()) 
        {

        };

        if(m_fence != nullptr)
        {
            RHI::get()->getFencePool().waitAndReleaseFence(m_fence,1000000);
            m_fence = nullptr;
        }
    }

    void GpuLoaderAsync::asyncTick()
    {
        const bool bNeedDispatch = RHI::get()->getFencePool().isFenceSignaled(m_fence) || m_bFirstTick;
        m_bFirstTick = false;

        static auto* textureManager = GEngine.getRuntimeModule<AssetSystem>()->getTextureManager();

        if(bNeedDispatch)
        {
            // process last tick finish callback.
            if(RHI::get()->getFencePool().isFenceSignaled(m_fence))
            {
                RHI::get()->getFencePool().resetFence(m_fence);
                vkResetCommandBuffer(m_cmdBuf,0);

                // update state.
                for(auto& uploadingTask : m_uploadingTaskes)
                {
                    uploadingTask.combineTexture->bReady = true;
                    uploadingTask.combineTexture->texture = uploadingTask.createImage;
                    uploadingTask.combineTexture->textureIndex =
                        textureManager->getBindless()->updateTextureToBindlessDescriptorSet(uploadingTask.createImage);
                }
                m_uploadingTaskes.clear();
                
                m_bProcessing = false;
            }

            // dipatch new.
            if(m_textureLoadTasks.size() > 0)
            {
                const auto totalSize =  getWholeStagingbufferSize();
                uint32_t usefulSize = totalSize;

                uint32_t offsetTextureBufferSize = 0;
                while(m_textureLoadTasks.size() > 0)
                {
                    TextureLoadTask processTask = m_textureLoadTasks.front();
                    auto uploadSize = processTask.textureInfo->binFileDataSize - processTask.textureInfo->rawFileDataSizeInBinFile;

                    if(usefulSize > uploadSize)
                    {
                        m_uploadingTaskes.push_back(processTask);
                        m_bProcessing = true;

                        // pop new gay.
                        m_textureLoadTasks.pop();

                        // load one texture.
                        TextureInfo& textureInfo = *processTask.textureInfo;

                        AssetBin assetBin {};
                        if(!loadBinFile(textureInfo.binFilePath.c_str(),assetBin))
                        {
                            LOG_FATAL("fail to load bin file! don't change project data.");
                        }

                        std::vector<uint8_t> texData = {};

                        texData.resize(textureInfo.binFileDataSize);
                        uncompressTextureBin(&textureInfo,assetBin.binBlob.data(),assetBin.binBlob.size(),(char*)texData.data());

                        // copy data to memory.
                        m_stageBuffer->map();
                        memcpy((void*)((char*)m_stageBuffer->mapped + offsetTextureBufferSize),
                            texData.data() + textureInfo.rawFileDataSizeInBinFile, 
                            uploadSize);

                        m_stageBuffer->unmap();

                        offsetTextureBufferSize += uploadSize;
                        usefulSize -= uploadSize;
                    }
                    else
                    {
                        break;
                    }
                }

                CHECK(m_uploadingTaskes.size() > 0);

                VkCommandBufferBeginInfo beginInfo{};
                beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
                vkBeginCommandBuffer(m_cmdBuf,&beginInfo);

                uint32_t offsetPos = 0;
                for(auto& uploadingTask : m_uploadingTaskes)
                {
                    TextureInfo& textureInfo = *uploadingTask.textureInfo;

                    const bool bBC3Compress = isBC3Format(textureInfo.format);
                    uint32_t mipWidth  = textureInfo.width;
                    uint32_t mipHeight = textureInfo.height;

                    for(uint32_t level = 0; level < textureInfo.mipmapLevelsCount; level++)
                    {
                        CHECK(mipWidth >= 1 && mipHeight >= 1);
                        uint32_t currentMipLevelSize = mipWidth * mipHeight * getTextureComponentCount(textureInfo.format);

                        if(bBC3Compress)
                        {
                            currentMipLevelSize = getTotoalBC3PixelCount(mipWidth,mipHeight);
                        }

                        VkBufferImageCopy region{};
                        region.bufferOffset = offsetPos;
                        region.bufferRowLength = 0;
                        region.bufferImageHeight = 0;
                        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                        region.imageSubresource.mipLevel = level;
                        region.imageSubresource.baseArrayLayer = 0;
                        region.imageSubresource.layerCount = 1;
                        region.imageOffset = {0, 0, 0};
                        region.imageExtent = {
                            mipWidth,
                            mipHeight,
                            1
                        };
                        vkCmdCopyBufferToImage(m_cmdBuf,*m_stageBuffer, uploadingTask.createImage->getImage(),VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&region);

                        if(level == (textureInfo.mipmapLevelsCount - 1))
                        {
                            uploadingTask.createImage->transitionLayout(m_cmdBuf,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_IMAGE_ASPECT_COLOR_BIT);
                        }

                        mipWidth  /= 2;
                        mipHeight /= 2;

                        offsetPos += currentMipLevelSize;
                    }
                }

                CHECK(offsetPos == offsetTextureBufferSize);

                vkEndCommandBuffer(m_cmdBuf);
                VkSubmitInfo submitInfo{};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &m_cmdBuf;

                vkQueueSubmit(RHI::get()->getAsyncCopyQueue(),1,&submitInfo,m_fence->getFence());
            } 
        }
    }

    void GpuLoaderAsync::addTextureLoadTask(const TextureLoadTask& in)
    {
        m_textureLoadTasks.push(in);
        m_wakeCondition.notify_one();
    }
}