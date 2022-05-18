#ifndef SSAO_COMMON_GLSL
#define SSAO_COMMON_GLSL

#include "Common.glsl"

// simple GTAO implement.
// you can see more detail from https://github.com/MaxwellGengYF/Unity-Ground-Truth-Ambient-Occlusion.
// 
// Four pass:
// pass #0: GTAO compute.
// pass #1: temporal accumulate.
// pass #2: spatial filter. X and Y Direction.

#define FRAME_DATA_SET 0
#include "FrameData.glsl"

#define VIEW_DATA_SET 1
#include "ViewData.glsl"

#define BLUE_NOISE_SET 3
#include "BlueNoiseCommon.glsl"

// linear sampler.
layout (set = 2, binding = 0) uniform sampler2D inDepth;      // GBuffer D: r32 unorm, .r is device z.
layout (set = 2, binding = 1) uniform sampler2D inGBufferS;   // GBuffer S: r8g8b8a8 unorm, .r is metal, .g is roughness, .b is mesh ao.
layout (set = 2, binding = 2) uniform sampler2D inGBufferB;   // GBuffer B: r16g16b16a16 sfloat, .rgb store worldspace normal, .a is shading model id
layout (set = 2, binding = 3) uniform sampler2D inGBufferA;   // GBuffer A: r8g8b8a8 unorm, .rgb is basecolor.

layout (set = 2, binding = 4, rg8) uniform image2D GTAOImage; // GTAO UAV: r8g8 unorm, .r is AO, .g is SO. Write
layout (set = 2, binding = 5) uniform sampler2D inGTAO;       // GTAO SRV. Read.

layout (set = 2, binding = 6, rg8) uniform image2D GTAOTempFilter; // GTAO UAV: r8g8 unorm, .r is AO, .g is SO. Write
layout (set = 2, binding = 7) uniform sampler2D inGTAOTempFilter;  // GTAO SRV. Read.

layout (set = 2, binding = 8, rg8) uniform image2D GTAOTemporal; // GTAO UAV: r8g8 unorm, .r is AO, .g is SO. Write
layout (set = 2, binding = 9) uniform sampler2D inGTAOTemporal;  // GTAO SRV. Read.

layout (set = 2, binding = 10) uniform sampler2D inVelocity; 
layout (set = 2, binding = 11) uniform sampler2D inHiz;

////////////////////// Constants//////////
const float kPI = 3.14159265359f;
const float kPI2 = kPI * kPI;
const float kPIHalf = kPI * 0.5f;
////////////////////////////////////////

struct PushConstantStruct
{
	int aoCut;
    int  aoDirection;
    int  aoSteps;
    float aoRadius;
    float soStrength;
    float aoPower;
};

layout(push_constant) uniform constants
{   
    PushConstantStruct pushConstant;
};

////////// Config ////////////////////

#define SSAO_HIZ_COMP  r
#define SSAO_HIZ_LEVEL 0

int   kGTAODirectionNum = pushConstant.aoDirection;
int   kGTAOStepNum = pushConstant.aoSteps;
float kGTAORadius = pushConstant.aoRadius;
float kGTROIntensity = pushConstant.soStrength;
float kGTAOPower = pushConstant.aoPower;

const float kGTAOThickness = 1.0f;
const int   kGTAOBlurRadius = 8;
const float kGTAOBlurSharpeness = 0.2f;
const float kGTAOTemporalWeight = 0.95f;
const float kGTAOTemporalClampScale = 0.75f;

vec3 GTAOGetViewPosition(vec2 uv)
{
    float deviceZ = textureLod(inHiz, uv, SSAO_HIZ_LEVEL).SSAO_HIZ_COMP;

    vec4 posClip  = vec4(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f, deviceZ, 1.0f);
    vec4 posViewRebuild = viewData.camProjJitterInverse * posClip;
    posViewRebuild.xyz /= posViewRebuild.w;

	return posViewRebuild.xyz;
}

vec3 GTAOGetWorldPosition(vec2 uv)
{
    float deviceZ = textureLod(inHiz, uv, SSAO_HIZ_LEVEL).SSAO_HIZ_COMP;

    vec4 posClip  = vec4(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f, deviceZ, 1.0f);
    vec4 posViewRebuild = viewData.camInvertViewProjectionJitter * posClip;
    posViewRebuild.xyz /= posViewRebuild.w;

	return posViewRebuild.xyz;
}

vec3 GTAOGetViewNormal(vec2 uv)
{
    // world normal.
    vec4 normal = vec4(normalize(texture(inGBufferB, uv).rgb), 0.0f);

    // to view normal.
    return normalize((viewData.camView * normal).rgb);
}

// See Activision GTAO paper: https://www.activision.com/cdn/research/s2016_pbs_activision_occlusion.pptx
float GTAOOffsets(ivec2 pixelPos)
{
	return 0.25 * float((pixelPos.y - pixelPos.x) & 3);
}

float GTAONoise(ivec2 pixelPos)
{
	return fract(52.9829189 * fract(dot(vec2(pixelPos), vec2(0.06711056, 0.00583715))));
}

// See Activision GTAO paper: https://www.activision.com/cdn/research/s2016_pbs_activision_occlusion.pptx
float GTAOIntegrateArcCosWeight(vec2 h, float n)
{
	vec2 arc = -cos(2 * h - n) + cos(n) + 2 * h * sin(n);
	return 0.25 * (arc.x + arc.y);
}

// From Activision GTAO paper: https://www.activision.com/cdn/research/s2016_pbs_activision_occlusion.pptx
const float kGTAOOffsets[] = { 0.0f, 0.5f, 0.25f, 0.75f };
const float kGTAORotations[] = { 0.1666f, 0.8333, 0.5f, 0.6666, 0.3333, 0.0f };

float GTROConeConeIntersection(float arcLength0, float arcLength1, float angleBetweenCones)
{
	float angleDifference = abs(arcLength0 - arcLength1);
	float angleBlendAlpha = clamp((angleBetweenCones - angleDifference) / (arcLength0 + arcLength1 - angleDifference), 0, 1.0f);
	return smoothstep(0, 1, 1 - angleBlendAlpha);
}

float GTROReflectionOcclusion(vec3 bentNormal, vec3 reflectionVector, float roughness, float occlusionStrength)
{
	float bentNormalLength = length(bentNormal);

	float reflectionConeAngle = max(roughness, 0.04) * kPI;
	float unoccludedAngle = bentNormalLength * kPI * occlusionStrength;
	float angleBetween = acos(dot(bentNormal, reflectionVector) / max(bentNormalLength, 0.001));

	float reflectionOcclusion = GTROConeConeIntersection(reflectionConeAngle, unoccludedAngle, angleBetween);
	return mix(0, reflectionOcclusion, clamp((unoccludedAngle - 0.1) / 0.2, 0.0f, 1.0f));
}

// spatial filter from https://github.com/MaxwellGengYF/Unity-Ground-Truth-Ambient-Occlusion
////////////////////////////////////////////////////////////////////////////////////////////////
void getAORODepth(vec2 uv, inout vec2 AORO, inout float d, sampler2D AOTex)
{
	AORO = texture(AOTex, uv).rg;
	d = linearizeDepth(textureLod(inHiz, uv, SSAO_HIZ_LEVEL).SSAO_HIZ_COMP, viewData.camInfo.z, viewData.camInfo.w);
}

float crossBilateralWeight(float r, float d, float d0)
{
	const float blurSigma = kGTAOBlurRadius * 0.5;
	const float blurFalloff = 1 / (2.0f * blurSigma * blurSigma);

	float dz = (d0 - d) * viewData.camInfo.w * kGTAOBlurSharpeness / 100.0f;
	return exp2(-r * r * blurFalloff - dz * dz);
}

void processSample(vec3 AORODepth, float r, float d0, inout vec2 totalAOR, inout float totalW)
{
	float w = crossBilateralWeight(r, d0, AORODepth.z);
	totalW += w;
	totalAOR += w * AORODepth.xy;
}

void processRadius(vec2 uv0, vec2 deltaUV, float d0, inout vec2 totalAORO, inout float totalW, sampler2D AOTex)
{
	int r = 1;

	float z = 0.0;
	vec2 uv = vec2(0.0);
	vec2 AORO = vec2(0.0);

	for (; r <= kGTAOBlurRadius / 2; r += 1) 
	{
		uv = uv0 + r * deltaUV;
		getAORODepth(uv, AORO, z, AOTex);
		processSample(vec3(AORO, z), float(r), d0, totalAORO, totalW);
	}

	for (; r <= kGTAOBlurRadius; r += 2) 
	{
		uv = uv0 + (r + 0.5) * deltaUV;
		getAORODepth(uv, AORO, z, AOTex);
		processSample(vec3(AORO, z), float(r), d0, totalAORO, totalW);
	}
}

vec3 bilateralBlur(vec2 uv, vec2 deltaUV, sampler2D AOTex)
{
	float depth;
	vec2 totalAOR;
	getAORODepth(uv, totalAOR, depth, AOTex);

	float totalW = 1;

	processRadius(uv, -deltaUV, depth, totalAOR, totalW, AOTex);
	processRadius(uv,  deltaUV, depth, totalAOR, totalW, AOTex);

	totalAOR /= totalW;
	return vec3(totalAOR, depth);
}
////////////////////////////////////////////////////////////////////////////////////////////////

#endif