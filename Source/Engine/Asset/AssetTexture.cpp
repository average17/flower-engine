#include "AssetTexture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb/stb_image_resize.h>

#define STB_DXT_IMPLEMENTATION
#include <stb/stb_dxt.h>

#include <nlohmann/json.hpp>
#include <lz4/lz4.h>
#include "../RHI/RHI.h"
#include "AssetSystem.h"
#include "../UI/UISystem.h"
#include "../UI/Icon.h"
#include "../Core/UUID.h"
#include "../RHI/Bindless.h"
#include "../Async/Taskflow.h"
#include "../RHI/ObjectUpload.h"
#include "../Renderer/DeferredRenderer/SceneTextures.h"

namespace flower
{
    engineImages::EngineImageInfo engineImages::white = {
        .name = "Image/T_White.tga",
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .component = 4,
        .bFlip = false,
        .bGenMipmap = true,
        .samplerType = ESamplerType::LinearRepeat
    };

    engineImages::EngineImageInfo engineImages::black = {
        .name = "Image/T_Black.tga",
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .component = 4,
        .bFlip = false,
        .bGenMipmap = true,
        .samplerType = ESamplerType::LinearRepeat
    };

    engineImages::EngineImageInfo engineImages::defaultNormal = {
        .name = "Image/T_DefaultNormal.tga",
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .component = 4,
        .bFlip = false,
        .bGenMipmap = true,
        .samplerType = ESamplerType::LinearRepeat
    };

    engineImages::EngineImageInfo engineImages::fallbackCapture = {
       .name = "Image/T_Fallback.hdr",
       .format = StaticTextures::getSpecularCaptureFormat(),
       .component = 4,
       .bHdr = true,
       .bFlip = true,
       .bGenMipmap = true,
       .samplerType = ESamplerType::LinearRepeat
    };

	Texture2DImage* loadTextureFromFile(const std::string& path,VkFormat format,uint32_t req,bool flip,bool bGenMipmaps)
    {
        int32_t texWidth, texHeight, texChannels;
        stbi_set_flip_vertically_on_load(flip);  
        stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels,req);

        if (!pixels) 
        {
            LOG_IO_FATAL("Fail to load texture {0}.",path);
        }

        int32_t imageSize = texWidth * texHeight * req;

        std::vector<uint8_t> pixelData{};
        pixelData.resize(imageSize);

        memcpy(pixelData.data(),pixels,imageSize);

        stbi_image_free(pixels);
        stbi_set_flip_vertically_on_load(false);  

        Texture2DImage* ret = Texture2DImage::createAndUpload(
            RHI::get()->getVulkanDevice(),
            RHI::get()->getGraphicsCommandPool(),
            RHI::get()->getGraphicsQueue(),
            pixelData,
            texWidth,
            texHeight,
            VK_IMAGE_ASPECT_COLOR_BIT,
            format,
            VK_IMAGE_ASPECT_COLOR_BIT,
            false,
            bGenMipmaps ? -1 : 1
        );

        return ret;
    }

    Texture2DImage* loadFromFileHdr(const std::string& path, VkFormat format, uint32_t req, bool flip, bool bGenMipmaps = true)
    {
        int32_t texWidth, texHeight, texChannels;
        stbi_set_flip_vertically_on_load(flip);

        float* pixels = stbi_loadf(path.c_str(), &texWidth, &texHeight, &texChannels, req);

        if (!pixels)
        {
            LOG_IO_FATAL("Fail to load image {0}.", path);
        }

        int32_t imageSize = texWidth * texHeight * req * 4;

        std::vector<uint8_t> pixelData{};
        pixelData.resize(imageSize);

        memcpy(pixelData.data(), reinterpret_cast<uint8_t*>(pixels), imageSize);

        stbi_image_free(pixels);
        stbi_set_flip_vertically_on_load(false);

        Texture2DImage* ret = Texture2DImage::createAndUpload(
            RHI::get()->getVulkanDevice(),
            RHI::get()->getGraphicsCommandPool(),
            RHI::get()->getGraphicsQueue(),
            pixelData,
            texWidth,
            texHeight,
            VK_IMAGE_ASPECT_COLOR_BIT,
            format,
            VK_IMAGE_ASPECT_COLOR_BIT,
            false,
            bGenMipmaps ? -1 : 1
        );

        return ret;
    }

    uint32_t getTextureMipmapPixelCount(uint32_t width,uint32_t height,uint32_t componentCount, uint32_t mipMapCount)
    {
        if(mipMapCount == 0) return 0;
        if(componentCount == 0) return 0;

        // mip0
        auto mipPixelCount = width * height * componentCount;
        if(mipMapCount == 1) return mipPixelCount;

        uint32_t mipWidth = width;
        uint32_t mipHeight = height;

        for(uint32_t mipmapLevel = 1; mipmapLevel < mipMapCount; mipmapLevel++)
        {
            const uint32_t currentMipWidth  = mipWidth  > 1 ? mipWidth  / 2 : 1;
            const uint32_t currentMipHeight = mipHeight > 1 ? mipHeight / 2 : 1;

            // add greater mip level pixels.
            mipPixelCount += currentMipWidth * currentMipHeight * componentCount;

            if(mipWidth > 1) mipWidth   /= 2;
            if(mipHeight> 1) mipHeight /= 2;
        }
        return mipPixelCount;
    }

    uint32_t getTextureMipmapPixelCountBC3Compress(uint32_t texWidth,uint32_t texHeight,uint32_t mipmapCounts)
    {
        CHECK(texWidth >= 4);
        CHECK(texHeight >= 4);

        // BC3 compress 1/4.
        const auto totalMipCount = getMipLevelsCount(texWidth,texHeight);
        if(totalMipCount == mipmapCounts) // have final two levels. 
        {
            const uint32_t totalMipmapPixelCountWithoutFinal2Level = getTextureMipmapPixelCount(texWidth,texHeight,4,mipmapCounts);

            // final two levels compress as 4x4 block.
            return (totalMipmapPixelCountWithoutFinal2Level - 1 * 1 * 4 - 2 * 2 * 4 + 4 * 4 * 4 * 2 ) /4;
        }
        else if(mipmapCounts == totalMipCount-1) // no last levels.
        {
            const uint32_t totalMipmapPixelCountWithoutFinal2Level = getTextureMipmapPixelCount(texWidth,texHeight,4,mipmapCounts);
            return (totalMipmapPixelCountWithoutFinal2Level - 1*1*4 - 2*2*4 + 4*4*4) / 4;
        }
        else if(mipmapCounts == totalMipCount-2) // no last two levels.
        {
            return getTextureMipmapPixelCount(texWidth,texHeight,4,mipmapCounts) / 4;
        }

        return 0;
    }

    uint32_t getTotoalBC3PixelCount(uint32_t width,uint32_t height)
    {
        CHECK(width == height);

        uint32_t trueWidth  = width  >= 4 ? width  : 4;
        uint32_t trueHeight = height >= 4 ? height : 4;

        return trueWidth * trueHeight;// * 4 / 4;
    }

    inline unsigned char srgbToLinear(unsigned char inSrgb)
    {
        float srgb = inSrgb / 255.0f;
        srgb = glm::max(6.10352e-5f, srgb);
        float lin = srgb > 0.04045f ? glm::pow( srgb * (1.0f / 1.055f) + 0.0521327f, 2.4f ) : srgb * (1.0f / 12.92f);

        return unsigned char(lin * 255.0f);
    }

    inline unsigned char linearToSrgb(unsigned char inlin)
    {
        float lin = inlin / 255.0f;
        if(lin < 0.00313067f) return unsigned char(lin * 12.92f * 255.0f);
        float srgb = glm::pow(lin,(1.0f/2.4f)) * 1.055f - 0.055f;

        return unsigned char(srgb * 255.0f);
    }

    bool isBC3Format(ETextureFormat format)
    {
        switch(format)
        {
        case flower::ETextureFormat::T_R8G8B8A8_BC3:
        case flower::ETextureFormat::T_R8G8B8A8_SRGB_BC3:
            return true;
        }

        return false;
    }

    bool isSRGBFormat(ETextureFormat format)
    {
        switch(format)
        {
        case flower::ETextureFormat::T_R8G8B8A8_SRGB:
        case flower::ETextureFormat::T_R8G8B8A8_SRGB_BC3:
            return true;
        }

        return false;
    }

    float getAlphaCoverage(const unsigned char* data, uint32_t width, uint32_t height, float scale, int cutoff)
    {
        double val = 0;

        // 4 char to 1 uint32_t
        uint32_t *pImg = (uint32_t *)data;

        for (uint32_t y = 0; y < height; y++)
        {
            for (uint32_t x = 0; x < width; x++)
            {            
                // pImg++ to next pixel 4 char = 1 uint32_t
                uint8_t *pPixel = (uint8_t *)pImg++;

                // alpha in pixel[3]
                int alpha = (int)(scale * (float)pPixel[3]);

                if (alpha > 255) 
                    alpha = 255;

                // skip cutoff.
                if (alpha <= cutoff)
                    continue;

                val += alpha;
            }
        }

        return (float)(val / (height * width * 255));
    }

    void scaleAlpha(unsigned char* data, uint32_t width, uint32_t height, float scale)
    {
        uint32_t *pImg = (uint32_t*)data;

        for (uint32_t y = 0; y < height; y++)
        {
            for (uint32_t x = 0; x < width; x++)
            {
                uint8_t *pPixel = (uint8_t *)pImg++;

                int alpha = (int)(scale * (float)pPixel[3]);
                if (alpha > 255) alpha = 255;

                pPixel[3] = alpha;
            }
        }
    }

    static bool mipmapGenerate(
        const unsigned char* data,
        std::vector<unsigned char>& result, 
        uint32_t width,
        uint32_t height,
        uint32_t componentCount,
        bool bSrgb,
        bool bAlphaCoverage,
        float cutOff)
    {
        if(componentCount != 4) return false;
        if(width!=height){ return false; }
        if(width < 4) return false;
        if(height < 4) return false;
        if(width & (width - 1)) return false; //  cpu current only process power of 2 mipmap.
        if(height & (height -1)) return false; //  cpu current only process power of 2 mipmap.

        uint32_t mipmapCount = getMipLevelsCount(width,height);
        if(mipmapCount <= 1)
        {
            return false;
        }

        // prepare mipmap pixel size.
        uint32_t mipPixelSize = getTextureMipmapPixelCount(width,height,componentCount,mipmapCount);
        result.clear();
        result.resize(mipPixelSize);

        // mip 0.
        uint32_t mipWidth  = width;
        uint32_t mipHeight = height;

        float alphaCoverageMip0 = 1.0f;
        if(bAlphaCoverage)
        {
            if(cutOff < 1.0f)
            {
                alphaCoverageMip0 = getAlphaCoverage(data, mipWidth, mipHeight, 1.0f, (int)(cutOff * 255));
            }
        }

        // copy mip 0 to result.
        uint32_t mipPtrPos = width * height * componentCount;
        uint32_t lastMipPtrPos = 0;
        memcpy((void*)result.data(),(void*)data,size_t(mipPtrPos));

        // generate from mip 1
        for(uint32_t mipmapLevel = 1; mipmapLevel < mipmapCount; mipmapLevel++) 
        {
            CHECK(mipPtrPos < mipPixelSize);

            CHECK(mipWidth  > 1);
            CHECK(mipHeight > 1);

            const uint32_t lastMipWidth     = mipWidth;
            const uint32_t lastMipHeight    = mipHeight;

            const uint32_t currentMipWidth  = mipWidth  / 2; 
            const uint32_t currentMipHeight = mipHeight / 2; 

            // process current mip level's all pixels.
            for(uint32_t pixelPosX = 0; pixelPosX < currentMipWidth; pixelPosX ++)
            {
                for(uint32_t pixelPosY = 0; pixelPosY < currentMipHeight; pixelPosY ++)
                {
                    unsigned char* currentMipPixelOffset = result.data() + mipPtrPos + (pixelPosX + pixelPosY * currentMipWidth) * componentCount;
                    unsigned char* redComp   = currentMipPixelOffset;
                    unsigned char* greenComp = currentMipPixelOffset + 1;
                    unsigned char* blueComp  = currentMipPixelOffset + 2;
                    unsigned char* alphaComp = currentMipPixelOffset + 3;

                    // (0,0) pixel
                    const uint32_t lastMipPixelPosX00 = pixelPosX * 2;
                    const uint32_t lastMipPixelPosY00 = pixelPosY * 2;

                    // (1,0) pixel
                    const uint32_t lastMipPixelPosX10 = pixelPosX * 2 + 1;
                    const uint32_t lastMipPixelPosY10 = pixelPosY * 2 + 0;

                    // (0,1) pixel
                    const uint32_t lastMipPixelPosX01 = pixelPosX * 2 + 0;
                    const uint32_t lastMipPixelPosY01 = pixelPosY * 2 + 1;

                    // (1,1) pixel
                    const uint32_t lastMipPixelPosX11 = pixelPosX * 2 + 1;
                    const uint32_t lastMipPixelPosY11 = pixelPosY * 2 + 1;

                    // on vulkan, we just set image format is SRGB,
                    // so hardware will automatically transfer to linear, don't compute on precache.
                    constexpr bool bSRGBProcess = false;

                    const unsigned char* lastMipPixelOffset00 = result.data() + lastMipPtrPos + (lastMipPixelPosX00 + lastMipPixelPosY00 * lastMipWidth) * componentCount;
                    const unsigned char* lastMipPixelOffset10 = result.data() + lastMipPtrPos + (lastMipPixelPosX10 + lastMipPixelPosY10 * lastMipWidth) * componentCount;
                    const unsigned char* lastMipPixelOffset01 = result.data() + lastMipPtrPos + (lastMipPixelPosX01 + lastMipPixelPosY01 * lastMipWidth) * componentCount;
                    const unsigned char* lastMipPixelOffset11 = result.data() + lastMipPtrPos + (lastMipPixelPosX11 + lastMipPixelPosY11 * lastMipWidth) * componentCount;

                    const unsigned char redLastMipPixel_00    = bSRGBProcess ? srgbToLinear(*(lastMipPixelOffset00))     : *(lastMipPixelOffset00);
                    const unsigned char greenLastMipPixel_00  = bSRGBProcess ? srgbToLinear(*(lastMipPixelOffset00 + 1)) : *(lastMipPixelOffset00 + 1);
                    const unsigned char blueLastMipPixel_00   = bSRGBProcess ? srgbToLinear(*(lastMipPixelOffset00 + 2)) : *(lastMipPixelOffset00 + 2);
                    const unsigned char* alphaLastMipPixel_00 = lastMipPixelOffset00 + 3;

                    const unsigned char redLastMipPixel_10    = bSRGBProcess ? srgbToLinear(*(lastMipPixelOffset10))     : *(lastMipPixelOffset10);
                    const unsigned char greenLastMipPixel_10  = bSRGBProcess ? srgbToLinear(*(lastMipPixelOffset10 + 1)) : *(lastMipPixelOffset10 + 1);
                    const unsigned char blueLastMipPixel_10   = bSRGBProcess ? srgbToLinear(*(lastMipPixelOffset10 + 2)) : *(lastMipPixelOffset10 + 2);
                    const unsigned char* alphaLastMipPixel_10 = lastMipPixelOffset10 + 3;

                    const unsigned char redLastMipPixel_01    = bSRGBProcess ? srgbToLinear(*(lastMipPixelOffset01))     : *(lastMipPixelOffset01);
                    const unsigned char greenLastMipPixel_01  = bSRGBProcess ? srgbToLinear(*(lastMipPixelOffset01 + 1)) : *(lastMipPixelOffset01 + 1);
                    const unsigned char blueLastMipPixel_01   = bSRGBProcess ? srgbToLinear(*(lastMipPixelOffset01 + 2)) : *(lastMipPixelOffset01 + 2);
                    const unsigned char* alphaLastMipPixel_01 = lastMipPixelOffset01 + 3;

                    const unsigned char redLastMipPixel_11    = bSRGBProcess ? srgbToLinear(*(lastMipPixelOffset11))     : *(lastMipPixelOffset11);
                    const unsigned char greenLastMipPixel_11  = bSRGBProcess ? srgbToLinear(*(lastMipPixelOffset11 + 1)) : *(lastMipPixelOffset11 + 1);
                    const unsigned char blueLastMipPixel_11   = bSRGBProcess ? srgbToLinear(*(lastMipPixelOffset11 + 2)) : *(lastMipPixelOffset11 + 2);
                    const unsigned char* alphaLastMipPixel_11 = lastMipPixelOffset11 + 3;

                    *alphaComp = (*alphaLastMipPixel_00 + *alphaLastMipPixel_10 + *alphaLastMipPixel_01 + *alphaLastMipPixel_11) / 4;
                    *greenComp =  (greenLastMipPixel_00 + greenLastMipPixel_10 + greenLastMipPixel_01 + greenLastMipPixel_11) / 4;
                    *blueComp  = (blueLastMipPixel_00 + blueLastMipPixel_10  + blueLastMipPixel_01  + blueLastMipPixel_11) / 4;
                    *redComp   =  (redLastMipPixel_00  + redLastMipPixel_10 + redLastMipPixel_01 + redLastMipPixel_11) / 4;

                    if(bSRGBProcess)
                    {
                        *redComp   = linearToSrgb(*redComp);
                        *greenComp = linearToSrgb(*greenComp);
                        *blueComp  = linearToSrgb(*blueComp);
                    }
                }
            }

            // alpha coverage for current mip levels.
            if(bAlphaCoverage)
            {
                if(alphaCoverageMip0 < 1.0f)
                {
                    float ini = 0;
                    float fin = 10;

                    float mid;
                    float alphaPercentage;

                    // find best alpha coverage for mip-map.
                    int iter = 0;
                    for(; iter < 50; iter++)
                    {
                        mid = (ini + fin) / 2;

                        alphaPercentage = getAlphaCoverage(result.data() + mipPtrPos, currentMipWidth, currentMipHeight, mid, (int)(cutOff * 255));
                        
                        if (glm::abs(alphaPercentage - alphaCoverageMip0) < .001)
                            break;

                        if (alphaPercentage > alphaCoverageMip0)
                            fin = mid;

                        if (alphaPercentage < alphaCoverageMip0)
                            ini = mid;
                    }

                    // store scale alpha.
                    scaleAlpha(result.data() + mipPtrPos, currentMipWidth, currentMipHeight, mid);
                }
            }

            lastMipPtrPos = mipPtrPos;
            mipPtrPos += currentMipWidth * currentMipHeight * componentCount;

            mipWidth  /= 2;
            mipHeight /= 2;
        }

        return true;
    }

    static bool mipmapCompressBC3(std::vector<unsigned char>& inData,std::vector<unsigned char>& result,uint32_t width,uint32_t height, uint32_t componentCount = 4)
    {
        static const uint32_t blockSize = 4;

        uint32_t mipmapCount = getMipLevelsCount(width,height);
        CHECK(mipmapCount >= 3); // final two mip levels use final third miplevels data because smaller than 4x4.

        result.resize(getTextureMipmapPixelCountBC3Compress(width,height,mipmapCount));

        uint32_t mipWidth = width;
        uint32_t mipHeight = height;

        uint32_t mipPtrPos = 0;

        uint8_t block[64] {};
        uint8_t* outBuffer = result.data();
        for(uint32_t mipmapLevel = 0; mipmapLevel < mipmapCount; mipmapLevel++)
        {
            CHECK(mipPtrPos < (uint32_t)inData.size());
            CHECK(mipWidth  >= 1);
            CHECK(mipHeight >= 1);


            if(mipWidth>=4) // general mip levels which bigger than 4x4
            {
                for(uint32_t pixelPosY = 0; pixelPosY < mipHeight; pixelPosY += blockSize)
                {
                    for(uint32_t pixelPosX = 0; pixelPosX < mipWidth; pixelPosX += blockSize)
                    {
                        // extract 4x4 pixel data to block
                        uint32_t blockLocation = 0;
                        for(uint32_t j = 0; j < blockSize; j++)
                        {
                            for(uint32_t i = 0; i < blockSize; i++)
                            {
                                const uint32_t dimX = pixelPosX + i;
                                const uint32_t dimY = pixelPosY + j;

                                const uint32_t pixelLocation = mipPtrPos+(dimX+dimY*mipWidth)*componentCount;
                                unsigned char* dataStart = inData.data()+pixelLocation;

                                for(uint32_t k = 0; k<componentCount; k++)
                                {
                                    CHECK(blockLocation<64);
                                    block[blockLocation] = *dataStart; blockLocation++; dataStart++;
                                }
                            }
                        }

                        // compress
                        stb_compress_dxt_block(outBuffer,block,1,STB_DXT_HIGHQUAL);
                        outBuffer += 16; 
                    }
                }
            }
            else if(mipWidth==2 && mipHeight==2)
            {
                const uint32_t pixelLocation00 = mipPtrPos + (0 + 0 * mipWidth) * componentCount;
                const uint32_t pixelLocation01 = mipPtrPos + (0 + 1 * mipWidth) * componentCount;
                const uint32_t pixelLocation10 = mipPtrPos + (1 + 0 * mipWidth) * componentCount;
                const uint32_t pixelLocation11 = mipPtrPos + (1 + 1 * mipWidth) * componentCount;

                // 00 block
                for(uint32_t j = 0; j<2; j++)
                {
                    for(uint32_t i = 0; i<2; i++)
                    {
                        uint32_t blockIndex = (i + j * 4) * 4;
                        block[blockIndex + 0] = *(inData.data()  + pixelLocation00 + 0);
                        block[blockIndex + 1] = *(inData.data()  + pixelLocation00 + 1);
                        block[blockIndex + 2] = *(inData.data()  + pixelLocation00 + 2);
                        block[blockIndex + 3] = *(inData.data()  + pixelLocation00 + 3);
                    }
                }

                // 01 block
                for(uint32_t j = 0; j<2; j++)
                {
                    for(uint32_t i = 0; i<2; i++)
                    {
                        uint32_t blockIndex = (i+j*4)*4;
                        block[blockIndex+0] = *(inData.data()+pixelLocation01+0);
                        block[blockIndex+1] = *(inData.data()+pixelLocation01+1);
                        block[blockIndex+2] = *(inData.data()+pixelLocation01+2);
                        block[blockIndex+3] = *(inData.data()+pixelLocation01+3);
                    }
                }

                // 10 block
                for(uint32_t j = 0; j<2; j++)
                {
                    for(uint32_t i = 0; i<2; i++)
                    {
                        uint32_t blockIndex = (i+j*4)*4;
                        block[blockIndex+0] = *(inData.data()+pixelLocation10+0);
                        block[blockIndex+1] = *(inData.data()+pixelLocation10+1);
                        block[blockIndex+2] = *(inData.data()+pixelLocation10+2);
                        block[blockIndex+3] = *(inData.data()+pixelLocation10+3);
                    }
                }

                // 11 block
                for(uint32_t j = 0; j<2; j++)
                {
                    for(uint32_t i = 0; i<2; i++)
                    {
                        uint32_t blockIndex = (i+j*4)*4;
                        block[blockIndex+0] = *(inData.data()+pixelLocation11+0);
                        block[blockIndex+1] = *(inData.data()+pixelLocation11+1);
                        block[blockIndex+2] = *(inData.data()+pixelLocation11+2);
                        block[blockIndex+3] = *(inData.data()+pixelLocation11+3);
                    }
                }

                stb_compress_dxt_block(outBuffer,block,1,STB_DXT_HIGHQUAL);
                outBuffer += 16;
            }
            else if(mipWidth==1 && mipHeight==1)
            {
                uint8_t* dataStart = inData.data() + mipPtrPos;
                for(uint32_t i = 0; i < 64; i += 4)
                {
                    block[i + 0] = *(dataStart + 0);
                    block[i + 1] = *(dataStart + 1);
                    block[i + 2] = *(dataStart + 2);
                    block[i + 3] = *(dataStart + 3);
                }
                stb_compress_dxt_block(outBuffer,block,1,STB_DXT_HIGHQUAL);
                // outBuffer += 16; // no offset on last levels.
            }

            // bias pointer to next mip levels.
            mipPtrPos += mipWidth * mipHeight * componentCount;

            mipWidth  /= 2;
            mipHeight /= 2;
        }

        return true;
    }


    uint32_t getDefaultTextureComponentsCount()
    {
        return 4;
    }

    ETextureFormat getDefaultTextureFormat()
    {
        return ETextureFormat::T_R8G8B8A8;
    }

    ESamplerType getDefaultSamplerType()
    {
        return ESamplerType::LinearRepeat;
    }

    std::pair<VkSampler, uint32_t> getSamplerAndBindlessIndex(ESamplerType type)
    {
        SamplerFactory sfa{};

        switch (type)
        {
        case flower::ESamplerType::Min:
        case flower::ESamplerType::Max:
        {
            LOG_FATAL("Error sampler type");
        }
        break;
        case flower::ESamplerType::PointClamp:
        {
            sfa.buildPointClampEdgeSampler();
        }
        break;
        case flower::ESamplerType::PointRepeat:
        {
            sfa.buildPointRepeatSampler();
        }
        break;
        case flower::ESamplerType::LinearClamp:
        {
            sfa.buildLinearClampSampler();
        }
        break;
        case flower::ESamplerType::LinearRepeat:
        {
            sfa.buildLinearRepeatSampler();
        }
        break;
        default:
        {
            LOG_FATAL("Exist unhandle sampler type.");
        }
        break;
        }

        uint32_t index = 0;
        VkSampler sampler = RHI::get()->getSampler(sfa.getCreateInfo(), index);

        return { sampler, index };
    }

    bool writeTexture(
        TextureInfo* info, 
        void* binFileData, 
        void* metaBinData, 
        AssetMeta* inoutMeta, 
        AssetBin* inoutBin)
    {
        // type & version fill.
        inoutMeta->type = (uint32_t)EAssetType::Texture;
        inoutMeta->version = FLOWER_ASSET_VERSION;

        // fill meta json info.
        nlohmann::json textureMetadata;
        textureMetadata["format"] = (uint32_t)info->format;
        textureMetadata["width"] = info->width;
        textureMetadata["height"] = info->height;
        textureMetadata["depth"] = info->depth;
        textureMetadata["samplerType"] = (uint32_t)info->samplerType;
        textureMetadata["compressMode"] = (uint32_t)info->compressMode;
        textureMetadata["metaBinDataSize"] = info->metaBinDataSize;
        textureMetadata["binFileDataSize"] = info->binFileDataSize;
        textureMetadata["metaFilePath"] = info->metaFilePath;
        textureMetadata["binFilePath"] = info->binFilePath;
        textureMetadata["UUID"] = info->UUID;
        textureMetadata["cutoff"] = info->cutoff;
        textureMetadata["mipmapLevelsCount"] = info->mipmapLevelsCount;
        textureMetadata["bCacheMipmap"] = info->bCacheMipmap;
        textureMetadata["rawFileDataSizeInBinFile"] = info->rawFileDataSizeInBinFile;

        if(info->compressMode == ECompressMode::LZ4)  
        {
            // compress meta file binblob.
            {
                auto compressStaging = LZ4_compressBound((int)info->metaBinDataSize);
                inoutMeta->binBlob.resize(compressStaging);

                auto compressedSize = LZ4_compress_default(
                    (const char*)metaBinData,
                    inoutMeta->binBlob.data(),
                    (int)info->metaBinDataSize,
                    compressStaging
                );

                inoutMeta->binBlob.resize(compressedSize);
            }
            
            // compress bin file binblob.
            {
                auto compressStaging = LZ4_compressBound((int)info->binFileDataSize);
                inoutBin->binBlob.resize(compressStaging);

                auto compressedSize = LZ4_compress_default(
                    (const char*)binFileData,
                    inoutBin->binBlob.data(),
                    (int)info->binFileDataSize,
                    compressStaging
                );

                inoutBin->binBlob.resize(compressedSize);
            }
        }
        else
        {
            // store meta.
            inoutMeta->binBlob.resize(info->metaBinDataSize);
            memcpy(inoutMeta->binBlob.data(), (const char*)metaBinData, info->metaBinDataSize);

            // store bin.
            inoutBin->binBlob.resize(info->binFileDataSize);
            memcpy(inoutBin->binBlob.data(),  (const char*)binFileData, info->binFileDataSize);
        }
        std::string stringified = textureMetadata.dump();
        inoutMeta->json = stringified;
        return true;
    }

    bool readTextureInfo(AssetMeta* metaFile, TextureInfo* inoutInfo)
    {
        nlohmann::json textureMetadata = nlohmann::json::parse(metaFile->json);

        inoutInfo->format = ETextureFormat(textureMetadata["format"]);
        inoutInfo->width = textureMetadata["width"];
        inoutInfo->height = textureMetadata["height"];
        inoutInfo->depth = textureMetadata["depth"];
        inoutInfo->samplerType = ESamplerType(textureMetadata["samplerType"]);
        inoutInfo->compressMode = ECompressMode(textureMetadata["compressMode"]);
        inoutInfo->binFileDataSize = textureMetadata["binFileDataSize"];
        inoutInfo->metaBinDataSize = textureMetadata["metaBinDataSize"];
        inoutInfo->metaFilePath = textureMetadata["metaFilePath"];
        inoutInfo->binFilePath = textureMetadata["binFilePath"];
        inoutInfo->UUID = textureMetadata["UUID"];
        inoutInfo->cutoff = textureMetadata["cutoff"];
        inoutInfo->mipmapLevelsCount = textureMetadata["mipmapLevelsCount"];
        inoutInfo->bCacheMipmap = textureMetadata["bCacheMipmap"];
        inoutInfo->rawFileDataSizeInBinFile = textureMetadata["rawFileDataSizeInBinFile"];
        return true;
    }

    void uncompressTextureMetaBin(TextureInfo* info,const char* srcBuffer,size_t srcSize,char* dest)
    {
        if(info->compressMode == ECompressMode::LZ4)
        {
            LZ4_decompress_safe(srcBuffer,dest,(int)srcSize,(int)info->metaBinDataSize);
        }
        else
        {
            memcpy(dest,srcBuffer,srcSize);
        }
    }

    void uncompressTextureBin(TextureInfo* info,const char* srcBuffer,size_t srcSize,char* dest)
    {
        if(info->compressMode == ECompressMode::LZ4)
        {
            LZ4_decompress_safe(srcBuffer,dest,(int)srcSize,(int)info->binFileDataSize);
        }
        else
        {
            memcpy(dest,srcBuffer,srcSize);
        }
    }

    bool bakeTexture2DLDR(
        const char* pathIn,
        const char* pathMetaOut,
        const char* pathBinOut,
        ECompressMode compressMode,
        uint32_t reqComp,
        ETextureFormat format,
        ESamplerType samplerType,
        bool bGenMipmap,
        float cutoff)
    {
        CHECK(reqComp == 4 && "Current only handle 4 component image.");

        int32_t texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(pathIn, &texWidth, &texHeight, &texChannels, reqComp);

        if (!pixels) 
        {
            LOG_IO_FATAL("Fail to load image {0}.",pathIn);
        }

        const bool bSRGB = isSRGBFormat(format);

        std::vector<unsigned char> generateMips{};
        bool bGenerateMipsSucess = false;
        if(bGenMipmap)
        {
            bGenerateMipsSucess = mipmapGenerate((unsigned char*)pixels,generateMips,texWidth,texHeight,reqComp, bSRGB, cutoff < 1.0f, cutoff);
        }

        const bool bNeedBC3Compress = isBC3Format(format)&&bGenerateMipsSucess;
        std::vector<unsigned char> generateMipsCompressBC3{};
        bool bGpuCompressSucess = false;
        if(bNeedBC3Compress)
        {
            bGpuCompressSucess = mipmapCompressBC3(generateMips, generateMipsCompressBC3, texWidth, texHeight, reqComp);
        }

        TextureInfo info{};
        info.format = format;
        info.width = texWidth;
        info.height = texHeight;
        info.depth = 1;
        info.samplerType = samplerType;
        info.compressMode = compressMode;
        info.metaFilePath = pathMetaOut;
        info.binFilePath = pathBinOut;
        info.cutoff = cutoff;
        info.bCacheMipmap = bGenerateMipsSucess;
        info.mipmapLevelsCount = getMipLevelsCount(texWidth,texHeight);
        // produce uuid when bake raw texture to asset.
        info.UUID = getUUID();

        int32_t rawPixelsSize = texWidth * texHeight * reqComp;
        info.rawFileDataSizeInBinFile = rawPixelsSize;

        int32_t imageSize = rawPixelsSize; // raw data size.
        if(bGpuCompressSucess)
        {
            CHECK(getTextureMipmapPixelCountBC3Compress(texWidth,texHeight,info.mipmapLevelsCount) == (uint32_t)generateMipsCompressBC3.size());
            imageSize += (int32_t)generateMipsCompressBC3.size();
        }
        else if(bGenerateMipsSucess)
        {
            CHECK(getTextureMipmapPixelCount(texWidth,texHeight,reqComp,info.mipmapLevelsCount) == (uint32_t)generateMips.size());
            imageSize += (int32_t)generateMips.size();
        }

        // maybe we need store other infos.
        info.binFileDataSize = imageSize;
        info.metaBinDataSize = SNAPSHOT_SIZE * SNAPSHOT_SIZE * reqComp;

        AssetMeta metaFile{};
        AssetBin binFile{};

        void* snapShotData = malloc(info.metaBinDataSize);

        // all snapshot texture use for imgui ui texture.
        // format default set to unorm.

        if(isSRGBFormat(format))
        {
            stbir_resize_uint8_srgb_edgemode(
                pixels, texWidth, texHeight, 0,
                (unsigned char*)snapShotData, SNAPSHOT_SIZE, SNAPSHOT_SIZE, 0,
                reqComp, 4, 0, STBIR_EDGE_CLAMP);// WRAP/REFLECT/ZERO
        }
        else
        {
            stbir_resize_uint8(pixels, texWidth, texHeight, 0,
                (unsigned char*)snapShotData, SNAPSHOT_SIZE, SNAPSHOT_SIZE, 0, reqComp);
        }

        

        // for bin: current only store pixels, future we maybe store mipmap datas.
        // for meta: current only store snapshot.
        std::vector<uint8_t> mergeData = {};
        if(bGpuCompressSucess)
        {
            // merge raw pixel data with BC3 compress data.
            mergeData.resize(imageSize);

            memcpy(mergeData.data(), pixels, rawPixelsSize);
            memcpy(mergeData.data() + rawPixelsSize, generateMipsCompressBC3.data(), size_t(imageSize - rawPixelsSize));

            writeTexture(&info, mergeData.data(), snapShotData, &metaFile, &binFile);
        }
        else if(bGenerateMipsSucess)
        {
            mergeData.resize(imageSize);

            memcpy(mergeData.data(), pixels, rawPixelsSize);
            memcpy(mergeData.data() + rawPixelsSize, generateMips.data(), size_t(imageSize - rawPixelsSize));


            // merge raw pixel data with mipmap data.
            writeTexture(&info, mergeData.data(), snapShotData, &metaFile, &binFile);
        }
        else
        {
            writeTexture(&info, pixels, snapShotData, &metaFile, &binFile);
        }
        

        stbi_image_free(pixels);
        free(snapShotData);

        return saveMetaFile(pathMetaOut, metaFile) && saveBinFile(pathBinOut, binFile);
    }

    VkFormat toVkFormat(ETextureFormat in)
    {
        switch(in)
        {
        case flower::ETextureFormat::T_R8G8B8A8:
            return VK_FORMAT_R8G8B8A8_UNORM;

        case flower::ETextureFormat::T_R8G8B8A8_SRGB:
            return VK_FORMAT_R8G8B8A8_SRGB;

        case flower::ETextureFormat::T_R8G8B8A8_BC3:
            return VK_FORMAT_BC3_UNORM_BLOCK;

        case flower::ETextureFormat::T_R8G8B8A8_SRGB_BC3:
            return VK_FORMAT_BC3_SRGB_BLOCK;

        default:
            return VK_FORMAT_R8G8B8A8_UNORM;
        }
    }

    uint32_t getTextureComponentCount(ETextureFormat in)
    {
        switch(in)
        {
        case flower::ETextureFormat::T_R8G8B8A8:
        case flower::ETextureFormat::T_R8G8B8A8_SRGB:
        case flower::ETextureFormat::T_R8G8B8A8_BC3:
        case flower::ETextureFormat::T_R8G8B8A8_SRGB_BC3:
            return 4;

        case flower::ETextureFormat::Min:
        case flower::ETextureFormat::Max:
        default:
            LOG_GRAPHICS_FATAL("Error format for texture.");
            return 0;
        }
    }

    void TextureManager::addTextureMetaInfo(AssetMeta* newMeta)
    {
        TextureInfo textureInfo{};
        readTextureInfo(newMeta, &textureInfo);

        // create snap shot image on texture manager.
        m_assetSystem->getUISystem()->getIconLibrary()->loadSnapShotFromTextureMetaFileLDR(
            textureInfo.metaFilePath,
            newMeta,
            &textureInfo
        );

        // update asset type map and meta file map.
        CHECK(!m_assetTextureMap.contains(textureInfo.metaFilePath));
        m_assetTextureMap[textureInfo.metaFilePath] = textureInfo;

        CHECK(!m_uuidMap.contains(textureInfo.UUID));
        m_uuidMap[textureInfo.UUID] = textureInfo.metaFilePath;
    }

    void TextureManager::erasedAssetMetaFile(std::string path)
    {
        if (m_assetTextureMap.contains(path))
        {
            // release snap shot image on texture manager.
            m_assetSystem->getUISystem()->getIconLibrary()->releaseSnapShot(path);

            // erase uuid key
            CHECK(m_uuidMap.contains(m_assetTextureMap[path].UUID));
            m_uuidMap.erase(m_assetTextureMap[path].UUID);

            // also remove texture info map.
            m_assetTextureMap.erase(path);
        }
    }

    CombineTextureBindless* TextureManager::getEngineImage(const engineImages::EngineImageInfo& info)
    {
        if (m_imagesMap.contains(info.name))
        {
            return m_imagesMap[info.name].get();
        }

        CHECK(std::filesystem::exists(info.name));

        auto combineTexture = std::make_unique<CombineTextureBindless>();

        // when loading engine texture, we blocking graphics queue.
        // because engine texture is use for fallback.

        Texture2DImage* imageNew;
        if (info.bHdr)
        {
            imageNew = loadFromFileHdr(info.name, info.format, info.component, info.bFlip, info.bGenMipmap);
        }
        else
        {
            imageNew = loadTextureFromFile(info.name, info.format, info.component, info.bFlip, info.bGenMipmap);
        }
        uint32_t bindlessIndex = m_bindlessTextures->updateTextureToBindlessDescriptorSet(imageNew);

        combineTexture->bReady = true;
        combineTexture->textureIndex = bindlessIndex;
        combineTexture->texture = imageNew;
        combineTexture->samplerIndex = getSamplerAndBindlessIndex(info.samplerType).second;

        m_imagesMap[info.name] = std::move(combineTexture);
        return m_imagesMap[info.name].get();
    }

    // loading block.
    CombineTextureBindless* TextureManager::getAssetImageByUUID(const std::string& uuid)
    {
        if (m_imagesMap.contains(uuid))
        {
            return m_imagesMap[uuid].get();
        }

        std::string orignalPath = m_uuidMap.at(uuid);
        TextureInfo& textureInfo = m_assetTextureMap.at(orignalPath);
        
        AssetBin assetBin {};
        if(!loadBinFile(textureInfo.binFilePath.c_str(),assetBin))
        {
            return nullptr;
        }

        std::vector<uint8_t> texData = {};
        texData.resize(textureInfo.binFileDataSize);
        uncompressTextureBin(&textureInfo,assetBin.binBlob.data(),assetBin.binBlob.size(),(char*)texData.data());
        
        auto newTex = std::make_unique<CombineTextureBindless>();

        newTex->bReady = true;
        newTex->cutoff = textureInfo.cutoff;
        newTex->samplerIndex = getSamplerAndBindlessIndex(textureInfo.samplerType).second;
        
        if(textureInfo.bCacheMipmap)
        {
            newTex->texture = Texture2DImage::create(
                RHI::get()->getVulkanDevice(),
                textureInfo.width,
                textureInfo.height,
                VK_IMAGE_ASPECT_COLOR_BIT,
                toVkFormat(textureInfo.format),
                false,
                textureInfo.mipmapLevelsCount
            );

            uint32_t mipWidth  = textureInfo.width;
            uint32_t mipHeight = textureInfo.height;

            // offset to mipmap cache datas.
            uint32_t offsetPtr = textureInfo.rawFileDataSizeInBinFile; 
            const bool bBC3Compress = isBC3Format(textureInfo.format);

            for(uint32_t level = 0; level < textureInfo.mipmapLevelsCount; level++)
            {
                CHECK(mipWidth >= 1 && mipHeight >= 1);
                uint32_t currentMipLevelSize = mipWidth * mipHeight * getTextureComponentCount(textureInfo.format);

                if(bBC3Compress)
                {
                    currentMipLevelSize = getTotoalBC3PixelCount(mipWidth,mipHeight);
                }

                auto* stageBuffer = VulkanBuffer::create(
                    RHI::get()->getVulkanDevice(),
                    RHI::get()->getCopyCommandPool(),
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                    VMA_MEMORY_USAGE_CPU_TO_GPU,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    (VkDeviceSize)currentMipLevelSize,
                    (void*)(texData.data() + offsetPtr)
                );
                executeImmediately(RHI::get()->getDevice(),RHI::get()->getComputeCommandPool(),RHI::get()->getComputeQueue(),[&](VkCommandBuffer cmd)
                {

                    newTex->texture->copyBufferToImage(cmd,*stageBuffer,mipWidth,mipHeight,1,VK_IMAGE_ASPECT_COLOR_BIT,level);
                    
                    if(level==(textureInfo.mipmapLevelsCount - 1))
                    {
                        newTex->texture->transitionLayout(cmd,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_IMAGE_ASPECT_COLOR_BIT);
                    }
                });
                delete stageBuffer;

                mipWidth  /= 2;
                mipHeight /= 2;
                offsetPtr += currentMipLevelSize;
            }
        }
        else
        {
            LOG_GRAPHICS_INFO("Uploading texture {0} to GPU and generate mipmaps...", orignalPath);
            newTex->texture = Texture2DImage::createAndUpload(
                RHI::get()->getVulkanDevice(),
                RHI::get()->getGraphicsCommandPool(),
                RHI::get()->getGraphicsQueue(),
                texData,
                textureInfo.width,
                textureInfo.height,
                VK_IMAGE_ASPECT_COLOR_BIT,
                toVkFormat(textureInfo.format)
            );
        }

        // update to bindless set.
        newTex->textureIndex = m_bindlessTextures->updateTextureToBindlessDescriptorSet(newTex->texture);
        
        m_imagesMap[uuid] = std::move(newTex);
        return m_imagesMap[uuid].get();
    }

    // loading block.
    CombineTextureBindless* TextureManager::getAssetImageByMetaName(const std::string& metaName)
    {
        return getAssetImageByUUID(m_assetTextureMap.at(metaName).UUID);
    }

    CombineTextureBindless* TextureManager::getAssetImageByUUIDAsync(const std::string& uuid)
    {
        if (m_imagesMap.contains(uuid))
        {
            return m_imagesMap[uuid].get();
        }

        std::string orignalPath = m_uuidMap.at(uuid);
        TextureInfo& textureInfo = m_assetTextureMap.at(orignalPath);

        auto newTex = std::make_unique<CombineTextureBindless>();

        newTex->bReady = false;
        newTex->textureIndex = getEngineImage(engineImages::white)->textureIndex;
        newTex->texture = getEngineImage(engineImages::white)->texture;
        newTex->cutoff   = textureInfo.cutoff;
        newTex->samplerIndex = getSamplerAndBindlessIndex(textureInfo.samplerType).second;
        
        // if cache mipmap, load async.
        if(textureInfo.bCacheMipmap)
        {
            const auto mipSizeTotal = textureInfo.binFileDataSize - textureInfo.rawFileDataSizeInBinFile;

            auto* newImage = Texture2DImage::create(
                RHI::get()->getVulkanDevice(),
                textureInfo.width,
                textureInfo.height,
                VK_IMAGE_ASPECT_COLOR_BIT,
                toVkFormat(textureInfo.format),
                false,
                textureInfo.mipmapLevelsCount
            );

            newImage->transitionLayoutImmediately(
                RHI::get()->getGraphicsCommandPool(),
                RHI::get()->getGraphicsQueue(),
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            if(mipSizeTotal > GpuLoaderAsync::MAX_UPLOAD_SIZE) // bigger than aync load size, load blocking.
            {
                AssetBin assetBin {};
                if(!loadBinFile(textureInfo.binFilePath.c_str(),assetBin))
                {
                    return nullptr;
                }

                std::vector<uint8_t> texData = {};

                texData.resize(textureInfo.binFileDataSize);
                uncompressTextureBin(&textureInfo,assetBin.binBlob.data(),assetBin.binBlob.size(),(char*)texData.data());

                newTex->texture = newImage;

                uint32_t mipWidth  = textureInfo.width;
                uint32_t mipHeight = textureInfo.height;

                // offset to mipmap cache datas.
                uint32_t offsetPtr = textureInfo.rawFileDataSizeInBinFile; 
                const bool bBC3Compress = isBC3Format(textureInfo.format);

                for(uint32_t level = 0; level < textureInfo.mipmapLevelsCount; level++)
                {
                    CHECK(mipWidth >= 1 && mipHeight >= 1);
                    uint32_t currentMipLevelSize = mipWidth * mipHeight * getTextureComponentCount(textureInfo.format);

                    if(bBC3Compress)
                    {
                        currentMipLevelSize = getTotoalBC3PixelCount(mipWidth,mipHeight);
                    }

                    auto* stageBuffer = VulkanBuffer::create(
                        RHI::get()->getVulkanDevice(),
                        RHI::get()->getGraphicsCommandPool(),
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                        VMA_MEMORY_USAGE_CPU_TO_GPU,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        (VkDeviceSize)currentMipLevelSize,
                        (void*)(texData.data() + offsetPtr)
                    );
                    executeImmediately(RHI::get()->getDevice(),RHI::get()->getGraphicsCommandPool(),RHI::get()->getGraphicsQueue(),[&](VkCommandBuffer cmd)
                    {

                        newTex->texture->copyBufferToImage(cmd,*stageBuffer,mipWidth,mipHeight,1,VK_IMAGE_ASPECT_COLOR_BIT,level);

                        if(level==(textureInfo.mipmapLevelsCount - 1))
                        {
                            newTex->texture->transitionLayout(cmd,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_IMAGE_ASPECT_COLOR_BIT);
                        }
                    });
                    delete stageBuffer;

                    mipWidth  /= 2;
                    mipHeight /= 2;
                    offsetPtr += currentMipLevelSize;
                }

                newTex->bReady = true;
                // update to bindless set.
                newTex->textureIndex = m_bindlessTextures->updateTextureToBindlessDescriptorSet(newTex->texture);
            }
            else // size suitable, load async.
            {
                GpuLoaderAsync::TextureLoadTask loadTask;
                loadTask.combineTexture = newTex.get();
                loadTask.createImage = newImage;
                loadTask.textureInfo = &m_assetTextureMap.at(orignalPath);

                RHI::get()->getUploader()->addTextureLoadTask(loadTask);
            }
        }
        else // if no cache mipmap, runtime generate, blocking load.
        {
            AssetBin assetBin {};
            if(!loadBinFile(textureInfo.binFilePath.c_str(),assetBin))
            {
                return nullptr;
            }

            std::vector<uint8_t> texData = {};
            texData.resize(textureInfo.binFileDataSize);
            uncompressTextureBin(&textureInfo,assetBin.binBlob.data(),assetBin.binBlob.size(),(char*)texData.data());

            LOG_GRAPHICS_INFO("Uploading texture {0} to GPU and generate mipmaps...", orignalPath);
            newTex->texture = Texture2DImage::createAndUpload(
                RHI::get()->getVulkanDevice(),
                RHI::get()->getGraphicsCommandPool(),
                RHI::get()->getGraphicsQueue(),
                texData,
                textureInfo.width,
                textureInfo.height,
                VK_IMAGE_ASPECT_COLOR_BIT,
                toVkFormat(textureInfo.format)
            );
            newTex->bReady = true;
            // update to bindless set.
            newTex->textureIndex = m_bindlessTextures->updateTextureToBindlessDescriptorSet(newTex->texture);
        }

        m_imagesMap[uuid] = std::move(newTex);
        return m_imagesMap[uuid].get();
    }

    CombineTextureBindless* TextureManager::getAssetImageByMetaNameAsync(const std::string& metaName)
    {
        return getAssetImageByUUIDAsync(m_assetTextureMap.at(metaName).UUID);
    }

    void TextureManager::init(AssetSystem* in)
    {
        m_assetSystem = in;

        m_bindlessTextures = std::make_unique<BindlessTexture>();
        m_bindlessTextures->init();

        // load fallback engine images.
        getEngineImage(engineImages::white);
        getEngineImage(engineImages::black);
        getEngineImage(engineImages::defaultNormal);
        getEngineImage(engineImages::fallbackCapture);
    }

    void TextureManager::tick()
    {

    }

    void TextureManager::release()
    {   
        m_bindlessTextures->release();
        RHI::get()->getUploader()->blockFlush();

        // release engine images.
        for (auto& pair : m_imagesMap)
        {
            if(!pair.second->bReady)
            {
                RHI::get()->waitIdle();
            }

            CHECK(pair.second->bReady);
            delete pair.second->texture;
        }
    }
}