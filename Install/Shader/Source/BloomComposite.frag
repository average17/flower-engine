#version 460

#include "BloomCommon.glsl"

struct PushConstantData
{
    float u_weight;
    float u_intensity;
};

layout(push_constant) uniform block
{
	PushConstantData myPerFrame;
};

layout (location = 0) in vec2 inTexCoord;
layout (location = 0) out vec4 outColor;

layout(set=0, binding=0) uniform sampler2D inputSampler;

void main()
{
    // NOTE: Basic bloom don't do intensity scale.
    //       We add secondary bloom for stylized.

    #ifdef ALL_BLOOM
        outColor = myPerFrame.u_weight * texture( inputSampler, inTexCoord).rgba;
    #else
        outColor = myPerFrame.u_weight * myPerFrame.u_intensity * texture( inputSampler, inTexCoord).rgba;
    #endif
    
}
