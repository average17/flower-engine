#version 460

#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_samplerless_texture_functions : require

layout(push_constant) uniform PushConsts
{   
    uint kInputLevel; // 0 is from src depth.
};

#define COMMON_SAMPLER_SET 1
#include "CommonSamplerSet.glsl"

layout (set = 0, binding = 0, r32f) uniform image2D hizClosestImage;
layout (set = 0, binding = 1, r32f) uniform image2D hizFurthestImage;
layout (set = 0, binding = 2) uniform texture2D inDepth; 
layout (set = 0, binding = 3) uniform texture2D inSrcHizClosest; 
layout (set = 0, binding = 4) uniform texture2D inSrcHizFurthest; 

void hizClosestFurthestCompare(inout vec2 closestFurthest, ivec2 samplePos)
{
    float z0 = texelFetch(inSrcHizClosest, samplePos, 0).r;
    float z1 = texelFetch(inSrcHizFurthest, samplePos, 0).r;

    closestFurthest.x = max(closestFurthest.x, z0); // Reverse z, so max value is closest.
    closestFurthest.y = min(closestFurthest.y, z1); // Reverse z, so min value is furthest.
}

#include "Schedule.glsl"

// TODO: Also try unreal fill to power of two style hiz.
//       Which help hiz-ray-cast very accurate.
// TODO: Performance optimize, current work ~0.2 ms in 2k. can optimize to ~0.05 ms if use single pass hiz tech.
layout (local_size_x = 8, local_size_y = 8) in;
void main()
{
    uvec2 groupThreadId = remap8x8(gl_LocalInvocationIndex);
    uvec2 dispatchId = groupThreadId + gl_WorkGroupID.xy * 8;
    ivec2 workPos = ivec2(dispatchId);

    ivec2 hizSize = imageSize(hizClosestImage); // Same with hizFurthestImage size.
    // border check.
    if(workPos.x >= hizSize.x || workPos.y >= hizSize.y)
    {
        return;
    }

    const bool bFromDepth = (kInputLevel <= 0);
    if(bFromDepth) // Copy to src mip 0.
    {
        float z = texelFetch(inDepth, workPos, 0).r;
        imageStore(hizClosestImage,  workPos, vec4(z, 0.0f, 0.0f, 0.0f));
        imageStore(hizFurthestImage, workPos, vec4(z, 0.0f, 0.0f, 0.0f));
        return;
    }

    ivec2 srcDim = ivec2(textureSize(inSrcHizClosest, 0));
    ivec2 outDim = hizSize;

    // Center sample uv.
    ivec2 basicSamplePOs = workPos * 2;

    // 3x3 tent to help depth reduce more balance. make ray cast acculturate.
    vec2 closestFurthest = vec2(0.0, 1.0);

    hizClosestFurthestCompare(closestFurthest, basicSamplePOs + ivec2(0, 0));
    hizClosestFurthestCompare(closestFurthest, basicSamplePOs + ivec2(0, 1));
    hizClosestFurthestCompare(closestFurthest, basicSamplePOs + ivec2(1, 0));
    hizClosestFurthestCompare(closestFurthest, basicSamplePOs + ivec2(1, 1));


	vec2 ratio = vec2(srcDim) / vec2(outDim);

    bool needExtraSampleX = ratio.x > 2.0;
    bool needExtraSampleY = ratio.y > 2.0;
    
    // Extra sample for odd size src downsample.
    // NOTE: In order to keep balance for screen-sapce ray cast, must to sample 3x3 tent each pixel.
    if(needExtraSampleX)
    {
        hizClosestFurthestCompare(closestFurthest, basicSamplePOs + ivec2(2, 1));
        hizClosestFurthestCompare(closestFurthest, basicSamplePOs + ivec2(2, 0));
    }
    if(needExtraSampleY)
    {
        hizClosestFurthestCompare(closestFurthest, basicSamplePOs + ivec2(0, 2));
        hizClosestFurthestCompare(closestFurthest, basicSamplePOs + ivec2(1, 2));
    }
    if(needExtraSampleX && needExtraSampleY)
    {
        hizClosestFurthestCompare(closestFurthest, basicSamplePOs + ivec2(2, 2));
    }

    imageStore(hizClosestImage, workPos, vec4(closestFurthest.x, 0.0f, 0.0f, 0.0f));
    imageStore(hizFurthestImage, workPos, vec4(closestFurthest.y, 0.0f, 0.0f, 0.0f));
}

