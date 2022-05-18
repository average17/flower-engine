#version 460
#extension GL_EXT_nonuniform_qualifier : require

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

layout(location = 0) in vec2 inUV0;
layout(location = 1) in vec3 inWorldNormal;
layout(location = 2) in vec3 inWorldPos;
layout(location = 3) in vec4 inTangent;
layout(location = 4) in vec4 inPrevPosNoJitter;
layout(location = 5) in vec4 inCurPosNoJitter;
layout(location = 6) in flat uint inObjectId;

// GBuffer A: r8g8b8a8 unorm, .rgb store base color
layout(location = 0) out vec4 outGBufferA;

// GBuffer B: r16g16b16a16 sfloat, .rgb store worldspace normal, .a is shading model id
layout(location = 1) out vec4 outGBufferB;

// GBuffer C: r16g16b16a16 sfloat, .rgb store emissive color
layout(location = 2) out vec4 outGBufferC;

// GBuffer S: r8g8b8a8 unorm, .r is metal, .g is roughness, .b is mesh ao.
layout(location = 3) out vec4 outGBufferS;

// Velocity
layout(location = 4) out vec2 outVelocity;

vec3 getNormalFromVertexAttribute(vec3 tangentVec3) 
{
    vec3 n = normalize(inWorldNormal);
    vec3 t = normalize(inTangent.xyz);
    vec3 b = normalize(cross(n,t) * inTangent.w);
    mat3 tbn =  mat3(t, b, n);
    return normalize(tbn * normalize(tangentVec3));
}

// "Improved Geometric Specular Antialiasing"
float specularAA(vec3 N, float r)
{
    float kappa = 0.18f; //threshold
    float pixelVariance = 0.5f;
    float pxVar2 = pixelVariance * pixelVariance;

    vec3 N_U = dFdxFine(N);
    vec3 N_V = dFdyFine(N);
    
    //squared lengths
    float lengthN_U2 = dot(N_U, N_U);
    float lengthN_V2 = dot(N_V, N_V);

    float variance = pxVar2 * (lengthN_V2 + lengthN_U2);    
    float kernelRoughness2 = min(2.f * variance, kappa);
    float rFiltered = clamp(sqrt(r * r + kernelRoughness2), 0.f, 1.f);
    return rFiltered;
}

void main() 
{
    PerObjectMaterialData material = materials[inObjectId];

    vec4 baseColor = tex(material.baseColorId, material.baseColorSampler, inUV0);
    if(baseColor.a < material.cutoff)
    {
        discard;
    }
    vec3 color =  baseColor.rgb;

    vec4 normalTex   = tex(material.normalTexId,   material.normalSampler,   inUV0);
    vec4 specularTex = tex(material.specularTexId, material.specularSampler, inUV0);
    vec4 emissiveTex = tex(material.emissiveTexId, material.emissiveSampler, inUV0);

    vec3 normalTan = normalTex.xyz;
    normalTan.y = 1.0f - normalTan.y;
    normalTan.xy = normalTan.xy * 2.0f - vec2(1.0f);

    normalTan.z = sqrt(max(0.0f, 1.0f - dot(normalTan.xy,normalTan.xy)));
    vec3 worldNormal = getNormalFromVertexAttribute(normalTan);

    
    outGBufferA.rgb = color; // base color

    outGBufferB.rgb = worldNormal;
    outGBufferB.a   = material.shadingModel;

    outGBufferC.rgb = emissiveTex.rgb;

    outGBufferS.r = specularTex.b; // metal

    outGBufferS.g = specularAA(worldNormal, specularTex.g); // roughness

    // TODO: Pack Mesh AO on specularTex.r
    outGBufferS.b = 1.0f; // specularTex.r; // ao

    // remove camera jitter velocity factor. 
    vec2 curPosNDC = inCurPosNoJitter.xy  /  inCurPosNoJitter.w;
    vec2 prePosNDC = inPrevPosNoJitter.xy / inPrevPosNoJitter.w;

    vec2 velocity = (curPosNDC - prePosNDC) * 0.5f;
    velocity.y *= -1.0f;

    outVelocity = velocity;
}