#version 460
#extension GL_EXT_nonuniform_qualifier : require

#define BINDLESS_IMAGE_SET 0
#define BINDLESS_SAMPLER_SET 1
#include "Bindless.glsl"

#define VIEW_DATA_SET 2
#include "ViewData.glsl"

#define FRAME_DATA_SET 3
#include "FrameData.glsl"
#include "SDSMCommon.glsl"
#include "Common.glsl"
layout(set = 4, binding = 0) readonly buffer PerObjectBuffer
{
	PerObjectData objects[];
};

layout(set = 5, binding = 0) readonly buffer PerObjectMaterialBuffer
{
	PerObjectMaterialData materials[];
};

layout(set = 6, binding = 0) readonly buffer DrawIndirectBuffer
{
	IndexedIndirectCommand indirectDraws[];
};

layout(set = 7, binding = 0) readonly buffer CascadeInfoBuffer
{
	CascadeInfo cascadeInfosbuffer[];
};

struct DepthPushConstants
{
	uint cascadeId;
	uint basicOffset;
};	

layout(push_constant) uniform constants
{   
   DepthPushConstants pushConstant;
};

layout (location = 0) in vec2 inUV0;
layout (location = 1) in flat uint inObjId;

layout(location = 0) out vec4 outColor;

void main() 
{
    PerObjectMaterialData material = materials[inObjId];
    vec4 baseColor = tex(material.baseColorId, material.baseColorSampler, inUV0);

    if(baseColor.a < material.cutoff)
    {
        discard;
    }

    outColor = vec4(0.0f);
}