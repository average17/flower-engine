#version 460

#include "TonemapperAMDFilter.glsl"

layout (location = 0) in  vec2 inUV0;
layout (location = 0) out vec4 outLDRColor;

layout (set = 0, binding = 0) uniform sampler2D inHDRSceneColor;

// Simple aces fit curve.
vec3 aces(vec3 x) 
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

//== ACESFitted ===========================
//  Baking Lab
//  by MJP and David Neubelt
//  http://mynameismjp.wordpress.com/
//  All code licensed under the MIT license
//=========================================

// sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
const mat3 ACESInputMat = mat3
(
    0.59719, 0.07600, 0.02840,
    0.35458, 0.90834, 0.13383,
    0.04823, 0.01566, 0.83777
);

// ODT_SAT => XYZ => D60_2_D65 => sRGB
const mat3 ACESOutputMat = mat3
(
    1.60475, -0.10208, -0.00327,
    -0.53108,  1.10813, -0.07276,
    -0.07367, -0.00605,  1.07602
);

// ACES filmic tone map approximation
// see https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl
vec3 RRTAndODTFit(vec3 color)
{
    vec3 a = color * (color + 0.0245786) - 0.000090537;
    vec3 b = color * (0.983729 * color + 0.4329510) + 0.238081;
    return a / b;
}

// Tone mapping 
vec3 toneMapACES(vec3 color)
{
    color = ACESInputMat * color;

    // Apply RRT and ODT
    color = RRTAndODTFit(color);
    color = ACESOutputMat * color;

    // Clamp to [0, 1]
    color = clamp(color, 0.0, 1.0);
    return color;
}

// Lottes 2016, "Advanced Techniques and Optimization of HDR Color Pipelines" - https://gpuopen.com/wp-content/uploads/2016/03/GdcVdrLottes.pdf
float TonemapLottes(float x, float contrast)
{
    const float a = 1.6 * contrast;
    const float d = 0.977;
    const float hdrMax = 8.0;
    const float midIn = 0.18;
    const float midOut = 0.267;

    // Can be precomputed
    const float b =
        (-pow(midIn, a) + pow(hdrMax, a) * midOut) /
        ((pow(hdrMax, a * d) - pow(midIn, a * d)) * midOut);
    const float c =
        (pow(hdrMax, a * d) * pow(midIn, a) - pow(hdrMax, a) * pow(midIn, a * d) * midOut) /
        ((pow(hdrMax, a * d) - pow(midIn, a * d)) * midOut);

    return pow(x, a) / (pow(x, a * d) * b + c);
}

vec3 TonemapLottesx3(vec3 c, float contrast) 
{
    return vec3(
        TonemapLottes(c.x, contrast), 
        TonemapLottes(c.y, contrast), 
        TonemapLottes(c.z, contrast)
    );
}

void main()
{
    vec4 hdrColor = texture(inHDRSceneColor, inUV0);

    hdrColor.rgb = AMDTonemapper(hdrColor.rgb);

    outLDRColor = hdrColor;
}