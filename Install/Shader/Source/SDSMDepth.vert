#version 460
#extension GL_EXT_nonuniform_qualifier : require

#define STANDARD_VERTEX_INPUT
#include "CommonVertex.glsl"

#define BINDLESS_IMAGE_SET 0
#define BINDLESS_SAMPLER_SET 1
#include "Bindless.glsl"

#define VIEW_DATA_SET 2
#include "ViewData.glsl"

#define FRAME_DATA_SET 3
#include "FrameData.glsl"

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

layout (location = 0) out vec2 outUV0;
layout (location = 1) out flat uint outObjId;

struct DepthPushConstants
{
	uint cascadeId;
};	

layout(push_constant) uniform constants
{   
   DepthPushConstants pushConstant;
};

void main()
{
	outUV0 = inUV0;

	uint Id = pushConstant.cascadeId * MAX_SSBO_OBJECTS + gl_DrawID;

	outObjId = indirectDraws[Id].objectId;

	mat4 modelMatrix = objects[outObjId].modelMatrix;
	mat4 mvp = cascadeInfosbuffer[pushConstant.cascadeId].cascadeViewProjMatrix * modelMatrix;

	gl_Position = mvp * vec4(inPosition, 1.0f);
}