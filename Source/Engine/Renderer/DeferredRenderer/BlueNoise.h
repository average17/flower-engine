#pragma once

#include "../../RHI/RHI.h"

// common blue noise sampler.
// 1024 x 1024, rgba8 unorm.
namespace flower
{
	inline VkFormat getBlueNoiseFormat()
	{
		return VK_FORMAT_R8G8B8A8_UNORM;
	}

    inline float getHaltonValue(int index, int radix)
    {
        float result = 0.f;
        float fraction = 1.f / radix;

        while (index > 0)
        {
            result += (index % radix) * fraction;
            index /= radix;
            fraction /= radix;
        }
        return result;
    }

    // fix size for engine prepared blue noise texture.

    // 128 / 16 = 8, 16 is Main TAA period.
    constexpr uint32_t BLUE_NOISE_DIM_XY = 128u;
    constexpr uint32_t BLUE_NOISE_SAMPLE_COUNT = 8u;

	// sample random offset for blue noise.
    inline glm::vec2 getHalton23RandomOffset(uint64_t renderIndex)
	{
        uint32_t sampleIndex = uint32_t(renderIndex % BLUE_NOISE_SAMPLE_COUNT);

        constexpr uint32_t ModSize = BLUE_NOISE_DIM_XY - 1;

        float x = getHaltonValue(sampleIndex & ModSize, 2);
        float y = getHaltonValue(sampleIndex & ModSize, 3);

        return { x, y };
	}
}