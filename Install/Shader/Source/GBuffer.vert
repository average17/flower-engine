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

layout(location = 0) out vec2 outUV0;
layout(location = 1) out vec3 outWorldNormal;
layout(location = 2) out vec3 outWorldPos;
layout(location = 3) out vec4 outTangent;

layout(location = 4) out vec4 outPrevPosNoJitter;
layout(location = 5) out vec4 outCurPosNoJitter;

layout(location = 6) out flat uint outObjectId;

void main()
{
    outUV0 = inUV0; 

    const uint objId = indirectDraws[gl_DrawID].objectId;
    outObjectId = objId;

    const mat4 modelMatrix = objects[objId].modelMatrix;
    outWorldPos = (modelMatrix * vec4(inPosition, 1.0f)).xyz;

    mat4 mvp = viewData.camViewProj * modelMatrix;
    outCurPosNoJitter = mvp * vec4(inPosition, 1.0f);

    mat4 curJitterMat = mat4(1.0f);
	curJitterMat[3][0] += frameData.jitterData.x;
	curJitterMat[3][1] += frameData.jitterData.y;
    gl_Position = curJitterMat * outCurPosNoJitter;

    mat4 prevTransMatrix = objects[objId].prevModelMatrix; 
	outPrevPosNoJitter = viewData.camViewProjLast * prevTransMatrix *  vec4(inPosition, 1.0f);

    mat3 normalMatrix = transpose(inverse(mat3(modelMatrix)));
    outWorldNormal = normalMatrix * normalize(inNormal);
    outTangent = vec4(normalMatrix * normalize(inTangent.xyz), inTangent.w);
}
