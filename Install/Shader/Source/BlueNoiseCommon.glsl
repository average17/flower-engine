#ifndef BLUE_NOISE_COMMON_GLSL
#define BLUE_NOISE_COMMON_GLSL

#ifndef BLUE_NOISE_SET
#define BLUE_NOISE_SET 0
#endif

float fmod(float a, float b)
{
  float c = fract(abs(a / b)) * abs(b);
  return (a < 0) ? -c : c;   /* if ( a < 0 ) c = 0-c */
}

// 16 spp blue noise.
layout(set = BLUE_NOISE_SET, binding = 0) readonly buffer sobolBuffer16Spp
{
    uint sobolBufferData16Spp[];
};
layout(set = BLUE_NOISE_SET, binding = 1) readonly buffer rankingTileBuffer16Spp
{
    uint rankingTileBufferData16Spp[];
};
layout(set = BLUE_NOISE_SET, binding = 2) readonly buffer scramblingTileBuffer16Spp
{
    uint scramblingTileBufferData16Spp[];
};

float SampleRandomNumber16Spp(uint pixelI, uint pixelJ, uint sampleIndex, uint sampleDimension) 
{
    // Wrap arguments
    pixelI = pixelI & 127u;
    pixelJ = pixelJ & 127u;
    sampleIndex = sampleIndex & 255u;
    sampleDimension = sampleDimension & 255u;

    // xor index based on optimized ranking
    const uint rankedSampleIndex = sampleIndex ^ rankingTileBufferData16Spp[sampleDimension + (pixelI + pixelJ * 128u) * 8u];

    // Fetch value in sequence
    uint value = sobolBufferData16Spp[sampleDimension + rankedSampleIndex * 256u];

    // If the dimension is optimized, xor sequence value based on optimized scrambling
    value = value ^ scramblingTileBufferData16Spp[(sampleDimension % 8u) + (pixelI + pixelJ * 128u) * 8u];

    // Convert to float and return
    return (value + 0.5f) / 256.0f;
}

// 1 spp.
layout(set = BLUE_NOISE_SET, binding = 3) readonly buffer sobolBuffer1Spp
{
    uint sobolBufferData1Spp[];
};
layout(set = BLUE_NOISE_SET, binding = 4) readonly buffer rankingTileBuffer1Spp
{
    uint rankingTileBufferData1Spp[];
};
layout(set = BLUE_NOISE_SET, binding = 5) readonly buffer scramblingTileBuffer1Spp
{
    uint scramblingTileBufferData1Spp[];
};

// Blue Noise Sampler by Eric Heitz. 
// Returns a value in the range [0, 1].
float SampleRandomNumber1Spp(uint pixelI, uint pixelJ, uint sampleIndex, uint sampleDimension) 
{
    // Wrap arguments
    pixelI = pixelI & 127u;
    pixelJ = pixelJ & 127u;
    sampleIndex = sampleIndex & 255u;
    sampleDimension = sampleDimension & 255u;

    // xor index based on optimized ranking
    const uint rankedSampleIndex = sampleIndex ^ rankingTileBufferData1Spp[sampleDimension + (pixelI + pixelJ * 128u) * 8u];

    // Fetch value in sequence
    uint value = sobolBufferData1Spp[sampleDimension + rankedSampleIndex * 256u];

    // If the dimension is optimized, xor sequence value based on optimized scrambling
    value = value ^ scramblingTileBufferData1Spp[(sampleDimension % 8u) + (pixelI + pixelJ * 128u) * 8u];

    // Convert to float and return
    return (value + 0.5f) / 256.0f;
}

// 4 spp.
layout(set = BLUE_NOISE_SET, binding = 6) readonly buffer sobolBuffer4Spp
{
    uint sobolBufferData4Spp[];
};
layout(set = BLUE_NOISE_SET, binding = 7) readonly buffer rankingTileBuffer4Spp
{
    uint rankingTileBufferData4Spp[];
};
layout(set = BLUE_NOISE_SET, binding = 8) readonly buffer scramblingTileBuffer4Spp
{
    uint scramblingTileBufferData4Spp[];
};

// Blue Noise Sampler by Eric Heitz. 
// Returns a value in the range [0, 1].
float SampleRandomNumber4Spp(uint pixelI, uint pixelJ, uint sampleIndex, uint sampleDimension) 
{
    // Wrap arguments
    pixelI = pixelI & 127u;
    pixelJ = pixelJ & 127u;
    sampleIndex = sampleIndex & 255u;
    sampleDimension = sampleDimension & 255u;

    // xor index based on optimized ranking
    const uint rankedSampleIndex = sampleIndex ^ rankingTileBufferData4Spp[sampleDimension + (pixelI + pixelJ * 128u) * 8u];

    // Fetch value in sequence
    uint value = sobolBufferData4Spp[sampleDimension + rankedSampleIndex * 256u];

    // If the dimension is optimized, xor sequence value based on optimized scrambling
    value = value ^ scramblingTileBufferData4Spp[(sampleDimension % 8u) + (pixelI + pixelJ * 128u) * 8u];

    // Convert to float and return
    return (value + 0.5f) / 256.0f;
}


#endif