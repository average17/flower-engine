#ifndef COMMON_GLSL
#define COMMON_GLSL

#include "Schedule.glsl"

/**
 * NOTE from filament engine: !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * 
 * Transforms a texture UV to make it suitable for a render target attachment.
 *
 * In Vulkan and Metal, texture coords are Y-down but in OpenGL they are Y-up. This wrapper function
 * accounts for these differences. When sampling from non-render targets (i.e. uploaded textures)
 * these differences do not matter because OpenGL has a second piece of backwardness, which is that
 * the first row of texels in glTexImage2D is interpreted as the bottom row.
 *
 * To protect users from these differences, we recommend that materials in the SURFACE domain
 * leverage this wrapper function when sampling from offscreen render targets.
 *
 */

#if 0
    // Importance things on screen uv which different from vulkan and opengl.
    vec2 clipToUV(vec2 uv) 
    {
        uv = uv * 0.5f + 0.5f;

    #if defined(VULKAN) || defined(METAL) || defined(DIRECTX)
        uv.y = 1.0 - uv.y; // Vulkan Metal And DirectX
    #endif
        return uv; // Open GL
    }

    vec2 uvToClip(vec2 uv) 
    {
    #if defined(VULKAN) || defined(METAL) || defined(DIRECTX)
        uv.y = 1.0 - uv.y; // Vulkan Metal And DirectX
    #endif

        uv = uv * 2.0f - 1.0f;
        return uv; // Open GL
    }
#endif

// NOTE: Engine's view space depth range is [-zFar, -zNear].
//       See linearizeDepth function and viewspaceDepth function.

//////////////////////////////////////////////////////////////////////////////////////////////////////

float remap(float v, float s, float e) { return (v - s) / (e - s); }
float remap(float x, float a, float b, float c, float d) { return (((x - a) / (b - a)) * (d - c)) + c; }

float linearstep(const float s, const float e, float v) { return clamp((v - s) / (e - s), 0., 1. );}
float linearstep0(const float e, float v) { return min(v / e, 1.0); }

float mean(vec2 v) { return dot(v, vec2(1.0f / 2.0f)); }
float mean(vec3 v) { return dot(v, vec3(1.0f / 3.0f)); }
float mean(vec4 v) { return dot(v, vec4(1.0f / 4.0f)); }

float saturate(float x) { return clamp(x, 0.0, 1.0); }
vec2  saturate(vec2  x) { return clamp(x, vec2(0.0), vec2(1.0)); }
vec3  saturate(vec3  x) { return clamp(x, vec3(0.0), vec3(1.0)); }
vec4  saturate(vec4  x) { return clamp(x, vec4(0.0), vec4(1.0)); }

// Saturated range, [0, 1] 
bool isSaturated(float x) { return x >= 0.0f && x <= 1.0f; }
bool isSaturated( vec2 x) { return isSaturated(x.x) && isSaturated(x.y); }
bool isSaturated( vec3 x) { return isSaturated(x.x) && isSaturated(x.y) && isSaturated(x.z);}
bool isSaturated( vec4 x) { return isSaturated(x.x) && isSaturated(x.y) && isSaturated(x.z) && isSaturated(x.w);}

// On range, [minV, maxV]
bool onRange(float x, float minV, float maxV) { return x >= minV && x <= maxV;}
bool onRange( vec2 x,  vec2 minV,  vec2 maxV) { return onRange(x.x, minV.x, maxV.x) && onRange(x.y, minV.y, maxV.y);}
bool onRange( vec3 x,  vec3 minV,  vec3 maxV) { return onRange(x.x, minV.x, maxV.x) && onRange(x.y, minV.y, maxV.y) && onRange(x.z, minV.z, maxV.z);}
bool onRange( vec4 x,  vec4 minV,  vec4 maxV) { return onRange(x.x, minV.x, maxV.x) && onRange(x.y, minV.y, maxV.y) && onRange(x.z, minV.z, maxV.z) && onRange(x.w, minV.w, maxV.w);}

#define kPI 3.141592653589793

const float kMaxHalfFloat = 65504.0f;
const float kMax11BitsFloat = 65024.0f;
const float kMax10BitsFloat = 64512.0f;
const vec3  kMax111110BitsFloat3 = vec3(kMax11BitsFloat, kMax11BitsFloat, kMax10BitsFloat);

// For 8-bit unorm texture, float error range = 1.0f / 255.0f = 0.004f
// Range value is about 1.275
#define kShadingModelRangeCheck 0.005

// Shading model count is 50.
// Step value is 0.02, about 5.1
#define kShadingModelUnvalid     0.0
#define kShadingModelStandardPBR 0.02

bool isInShadingModelRange(float v, float shadingModel)
{
    return (v > (shadingModel - kShadingModelRangeCheck)) &&
           (v < (shadingModel + kShadingModelRangeCheck));
}

bool isShadingModelValid(float v)
{
    return v > (kShadingModelUnvalid + kShadingModelRangeCheck);
}

vec2 vogelDiskSample(uint sampleIndex, uint sampleCount, float angle)
{
  const float goldenAngle = 2.399963f;
  float r = sqrt(sampleIndex + 0.5f) / sqrt(sampleCount);
  float theta = sampleIndex * goldenAngle + angle;
  float sine = sin(theta);
  float cosine = cos(theta);

  return vec2(cosine, sine) * r;
}

// Rounds value to the nearest multiple of 8
uvec2 roundUp8(uvec2 value) 
{
    uvec2 roundDown = value & ~0x7;
    return (roundDown == value) ? value : value + 8;
}

float luminance(vec3 color)
{
    // human eye aware lumiance function.
    return dot(color, vec3(0.299, 0.587, 0.114));
}

// Earth atmosphere info.
struct EarthAtmosphere // Color space 2020
{
	vec3  absorptionExtinction;
	float multipleScatteringFactor;  // x4

	vec3 rayleighScattering; 
	float miePhaseFunctionG; // x4

	vec3 mieScattering;  
	float bottomRadius; // x4

	vec3 mieExtinction;  
	float topRadius; // x4

	vec3 mieAbsorption;  
	uint viewRayMarchMinSPP; // x4

	vec3 groundAlbedo;  
	uint viewRayMarchMaxSPP; // x4

	vec4 rayleighDensity[3]; //
	vec4 mieDensity[3]; //
	vec4 absorptionDensity[3]; //


    float cloudAreaStartHeight; // km
    float cloudAreaThickness;
    float atmospherePreExposure;
    float pad1;
};

struct DirectionalLightInfo
{
    vec3  color; // color spec rec 2020.
    float intensity; // x4

    vec3 direction;
    float shadowFilterSize; // shadow filter size for pcf.

    uint cascadeCount;
    uint perCascadeXYDim;
    float splitLambda;
    float shadowBiasConst; // x4

	float shadowBiasSlope;
    float cascadeBorderAdopt;
    float cascadeEdgeLerpThreshold;
    float maxDrawDepthDistance;

    float maxFilterSize;
    float pad0;
    float pad1;
    float pad2;
};

struct TonemapperParam
{
    float tonemapper_P;  // Max brightness.
    float tonemapper_a;  // contrast
    float tonemapper_m; // linear section start
    float tonemapper_l;  // linear section length

    float tonemapper_c; // black
    float tonemapper_b;  // pedestal
    float tonemmaper_s;
    float pad1;

    uint shoulder;
    uint con;
    uint soft;
    uint con2;

    uint clip;
    uint scaleOnly;
    uint displayMode;
    uint bHdr10;

    mat4 inputToOutputMatrix;
    ivec4 ctl[24 * 4];
};

// One frame data, cache some common info for rendering.
struct FrameData
{
    vec4 appTime;
    // .x is app runtime
    // .y is sin(.x)
    // .z is cos(.x)
    // .w is pad

    uvec4 frameIndex;
    // .x is frame count
    // .y is frame count % 8
    // .z is frame count % 16
    // .w is frame count % 32

    uint jitterPeriod; // jitter period for jitter data.
    float basicTextureLODBias; // Lod basic texture bias when render mesh.
    uint staticMeshCount;
    uint bSdsmDraw;

	float pad0;
	uint  globalIBLEnable;
	float globalIBLIntensity;
	float pad2;


    vec4 jitterData;
    // Halton sequence jitter data.
    // .xy is current frame jitter data.
    // .zw is prev frame jitter data.

    uint directionalLightCount; // directional count, 0 or 1.
    uint pointLightCount;
    uint spotLightCount;
    uint rectLightCount;

    // Importance lights info.
    DirectionalLightInfo directionalLight;

    // atmosphere of current earth.
    EarthAtmosphere earthAtmosphere;

    TonemapperParam toneMapper;
};

// All units in kilometers
struct AtmosphereParameters
{
    float atmospherePreExposure;

	// Radius of the planet (center to ground)
	float bottomRadius;

	// Maximum considered atmosphere height (center to atmosphere top)
	float topRadius;

	// Rayleigh scattering exponential distribution scale in the atmosphere
	float rayleighDensityExpScale;

	// Rayleigh scattering coefficients
	vec3 rayleighScattering;

	// Mie scattering exponential distribution scale in the atmosphere
	float mieDensityExpScale;

	// Mie scattering coefficients
	vec3 mieScattering;

	// Mie extinction coefficients
	vec3 mieExtinction;

	// Mie absorption coefficients
	vec3 mieAbsorption;

	// Mie phase function excentricity
	float miePhaseG;

	// Another medium type in the atmosphere
	float absorptionDensity0LayerWidth;
	float absorptionDensity0ConstantTerm;
	float absorptionDensity0LinearTerm;
	float absorptionDensity1ConstantTerm;
	float absorptionDensity1LinearTerm;

	// This other medium only absorb light, e.g. useful to represent ozone in the earth atmosphere
	vec3 absorptionExtinction;

	// The albedo of the ground.
	vec3 groundAlbedo;

	float multipleScatteringFactor; 
	uint viewRayMarchMinSPP;
	uint viewRayMarchMaxSPP;

    float cloudAreaStartHeight; // km
    float cloudAreaThickness;
	
    
};

// All units in kilometers
AtmosphereParameters getAtmosphereParameters(in const FrameData frameData)
{
	AtmosphereParameters parameters;
	parameters.absorptionExtinction = frameData.earthAtmosphere.absorptionExtinction;

	// Traslation from Bruneton2017 parameterisation.
	parameters.rayleighDensityExpScale = frameData.earthAtmosphere.rayleighDensity[1].w;
	parameters.mieDensityExpScale = frameData.earthAtmosphere.mieDensity[1].w;
	parameters.absorptionDensity0LayerWidth = frameData.earthAtmosphere.absorptionDensity[0].x;
	parameters.absorptionDensity0ConstantTerm = frameData.earthAtmosphere.absorptionDensity[1].x;
	parameters.absorptionDensity0LinearTerm = frameData.earthAtmosphere.absorptionDensity[0].w;
	parameters.absorptionDensity1ConstantTerm = frameData.earthAtmosphere.absorptionDensity[2].y;
	parameters.absorptionDensity1LinearTerm = frameData.earthAtmosphere.absorptionDensity[2].x;

	parameters.miePhaseG = frameData.earthAtmosphere.miePhaseFunctionG;
	parameters.rayleighScattering = frameData.earthAtmosphere.rayleighScattering;
	parameters.mieScattering = frameData.earthAtmosphere.mieScattering;
	parameters.mieAbsorption = frameData.earthAtmosphere.mieAbsorption;
	parameters.mieExtinction = frameData.earthAtmosphere.mieExtinction;
	parameters.groundAlbedo = frameData.earthAtmosphere.groundAlbedo;
	parameters.bottomRadius = frameData.earthAtmosphere.bottomRadius;
	parameters.topRadius = frameData.earthAtmosphere.topRadius;

	parameters.multipleScatteringFactor = frameData.earthAtmosphere.multipleScatteringFactor;

	parameters.viewRayMarchMinSPP = frameData.earthAtmosphere.viewRayMarchMinSPP;
	parameters.viewRayMarchMaxSPP = frameData.earthAtmosphere.viewRayMarchMaxSPP;


    parameters.cloudAreaStartHeight = frameData.earthAtmosphere.cloudAreaStartHeight;
    parameters.cloudAreaThickness = frameData.earthAtmosphere.cloudAreaThickness;
    parameters.atmospherePreExposure = frameData.earthAtmosphere.atmospherePreExposure;

	return parameters;
}

// https://github.com/sebh/UnrealEngineSkyAtmosphere
// Transmittance LUT function parameterisation from Bruneton 2017 https://github.com/ebruneton/precomputed_atmospheric_scattering
// Detail also in video https://www.youtube.com/watch?v=y-oBGzDCZKI at 08:35.
void lutTransmittanceParamsToUv(
    in const AtmosphereParameters atmosphere, 
    in float viewHeight, // [bottomRAdius, topRadius]
    in float viewZenithCosAngle, // [-1,1]
    out vec2 uv) // [0,1]
{
	float H = sqrt(max(0.0f, atmosphere.topRadius * atmosphere.topRadius - atmosphere.bottomRadius * atmosphere.bottomRadius));
	float rho = sqrt(max(0.0f, viewHeight * viewHeight - atmosphere.bottomRadius * atmosphere.bottomRadius));

	uv.y = rho / H;

	// Distance to atmosphere boundary
	float discriminant = viewHeight * viewHeight * (viewZenithCosAngle * viewZenithCosAngle - 1.0) + atmosphere.topRadius * atmosphere.topRadius;
	float d = max(0.0, (-viewHeight * viewZenithCosAngle + sqrt(discriminant))); 

	float dMin = atmosphere.topRadius - viewHeight;
	float dMax = rho + H;

	uv.x = (d - dMin) / (dMax - dMin);
}

void uvToLutTransmittanceParams(
    in const AtmosphereParameters atmosphere, 
    out float viewHeight, // [bottomRAdius, topRadius]
    out float viewZenithCosAngle, // [-1,1]
    in vec2 uv) // [0,1]
{ 
	float H = sqrt(atmosphere.topRadius * atmosphere.topRadius - atmosphere.bottomRadius * atmosphere.bottomRadius);
	float rho = H * uv.y;
	viewHeight = sqrt(rho * rho + atmosphere.bottomRadius * atmosphere.bottomRadius);

	float dMin = atmosphere.topRadius - viewHeight;
	float dMax = rho + H;

	// Distance to atmosphere boundary
	float d = dMin + uv.x * (dMax - dMin);

	viewZenithCosAngle = (d == 0.0) ? 1.0f : (H * H - rho * rho - d * d) / (2.0 * viewHeight * d);
	viewZenithCosAngle = clamp(viewZenithCosAngle, -1.0, 1.0);
}


mat4 buildJitterMatrix(vec2 jitterData)
{
    mat4 jitterMatrix = mat4(1.0f);
	jitterMatrix[3][0] += jitterData.x;
	jitterMatrix[3][1] += jitterData.y;
    return jitterMatrix;
}

struct ViewData
{
    // Camera misc infos.
    vec4 camWorldPos;
    vec4 camInfo; // .x fovy, .y aspectRatio, .z nearZ, .w farZ
    vec4 camInfoPrev; // prev-frame's cam info.
    
    // Camera basic matrixs.
    mat4 camView;
    mat4 camProj;
    mat4 camViewProj;

    // Camera invert matrixs.
    mat4 camInvertView;
    mat4 camInvertProj;
    mat4 camInvertViewProj;

    // Matrix always no jitter effects.
    mat4 camProjNoJitter;
    mat4 camViewProjNoJitter;

    // Camera invert matrixs no jitter effects.
    mat4 camInvertProjNoJitter;
    mat4 camInvertViewProjNoJitter;

    // Prev camera infos.
    mat4 camViewProjPrev;
    mat4 camViewProjPrevNoJitter;

    // Camera frustum planes for culling.
    vec4 frustumPlanes[6];

    float cameraAtmosphereOffsetHeight;
    float cameraAtmosphereMoveScale;
    float exposure;
    float ev100;

    float evCompensation;
    float pad0;
    float pad1;
    float pad2;
};

bool cameraCut(in const FrameData frameData)
{
    // When camera cut, frame index reset to zero.
    return frameData.frameIndex.x == 0; 
}


// Camera unit to atmosphere unit convert.
vec3 convertToAtmosphereUnit(vec3 o, in const ViewData viewData)
{
	const float cameraOffset = viewData.cameraAtmosphereOffsetHeight;
	return o * 0.001f * viewData.cameraAtmosphereMoveScale + cameraOffset;
}  

vec3 convertToCameraUnit(vec3 o, in const ViewData viewData)
{
	const float cameraOffset = viewData.cameraAtmosphereOffsetHeight;

	vec3 o1 = o - cameraOffset;
	return o1 / (0.001f * viewData.cameraAtmosphereMoveScale);
}  

// Simple hash uint.
// from niagara stream. see https://www.youtube.com/watch?v=BR2my8OE1Sc
uint simpleHash(uint a)
{
   a = (a + 0x7ed55d16) + (a << 12);
   a = (a ^ 0xc761c23c) ^ (a >> 19);
   a = (a + 0x165667b1) + (a << 5);
   a = (a + 0xd3a2646c) ^ (a << 9);
   a = (a + 0xfd7046c5) + (a << 3);
   a = (a ^ 0xb55a4f09) ^ (a >> 16);
   return a;
}

// Simple hash color from uint value.
// from niagara stream. see https://www.youtube.com/watch?v=BR2my8OE1Sc
vec3 simpleHashColor(uint i)
{
    uint h = simpleHash(i);
    return vec3(float(h & 255), float((h >> 8) & 255), float((h >> 16) & 255)) / 255.0;
}

// Project position to uv space.
vec3 projectPos(vec3 origin, in const mat4 inMatrix)
{
    vec4 projectPos = inMatrix * vec4(origin, 1.0);
    projectPos.xyz /= projectPos.w;

    projectPos.xy = 0.5 * projectPos.xy + 0.5;
    projectPos.y  = 1.0 - projectPos.y;

    return projectPos.xyz;
}

// Construct position like view space or world space.
vec3 constructPos(vec2 uv, float depthZ, in const mat4 invertMatrix)
{
    vec4 posClip  = vec4(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f, depthZ, 1.0f);
    vec4 posWorldRebuild = invertMatrix * posClip;
    return posWorldRebuild.xyz / posWorldRebuild.w;
}

// Construct world position from device z and it's sample uv.
vec3 getWorldPos(vec2 uv, float depthZ, in const ViewData view)
{
    return constructPos(uv, depthZ, view.camInvertViewProj);
}

// Construct view space position from device z and it's sample uv.
vec3 getViewPos(vec2 uv, float depthZ, in const ViewData view)
{
    return constructPos(uv, depthZ, view.camInvertProj);
}

// Vulkan linearize z.
// NOTE: viewspace z range is [-zFar, -zNear], linear z is viewspace z mul -1 result on vulkan.
// if no exist reverse z:
//       linearZ = zNear * zFar / (zFar +  deviceZ * (zNear - zFar));
//  when reverse z enable, then the function is:
//       linearZ = zNear * zFar / (zNear + deviceZ * (zFar - zNear));
float linearizeDepth(float z, float n, float f)
{
    return n * f / (z * (f - n) + n);
}

float linearizeDepth(float z, in const ViewData view)
{
    const float n = view.camInfo.z;
    const float f = view.camInfo.w;
    return linearizeDepth(z, n, f);
}

float linearizeDepthPrev(float z, in const ViewData view)
{
    const float n = view.camInfoPrev.z;
    const float f = view.camInfoPrev.w;
    return linearizeDepth(z, n, f);
}

// Derived by glm::perspectiveRH_ZO, same with linearizeDepth function.
float viewspaceDepth(float z, float n, float f)
{
    return linearizeDepth(z, n, f) * -1.0f;
}

// Radical inverse based on http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
vec2 hammersley2d(uint i, uint N) 
{
    // Efficient VanDerCorpus calculation.
	uint bits = (i << 16u) | (i >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	float rdi = float(bits) * 2.3283064365386963e-10;

    // Hammersley sequence.
	return vec2(float(i) /float(N), rdi);
}

// high frequency dither pattern appearing almost random without banding steps
// note: from "NEXT GENERATION POST PROCESSING IN CALL OF DUTY: ADVANCED WARFARE"
//      http://advances.realtimerendering.com/s2014/index.html
float interleavedGradientNoise(vec2 uv, float frameId)
{
	// magic values are found by experimentation
	uv += frameId * (vec2(47.0, 17.0) * 0.695f);

    const vec3 magic = vec3(0.06711056f, 0.00583715f, 52.9829189f);
    return fract(magic.z * fract(dot(uv, magic.xy)));
}

// Some screen space effect fade factor on screen edge.
float screenFade(vec2 uv)
{
    vec2 fade = max(vec2(0.0f), 12.0f * abs(uv - 0.5f) - 5.0f);
    return clamp(1.0f - dot(fade, fade), 0.0f, 1.0f);
}

// Based omn http://byteblacksmith.com/improvements-to-the-canonical-one-liner-glsl-rand-for-opengl-es-2-0/
float random(vec2 co)
{
	float a = 12.9898;
	float b = 78.233;
	float c = 43758.5453;

	float dt = dot(co.xy ,vec2(a,b));
	float sn = mod(dt,3.14);
    
	return fract(sin(sn) * c);
}

float whangHashNoise(uint u, uint v, uint s)
{
	uint seed = (u * 1664525u + v) + s;
	seed = (seed ^ 61u) ^ (seed >> 16u);
	seed *= 9u;
	seed = seed ^ (seed >> 4u);
	seed *= uint(0x27d4eb2d);
	seed = seed ^ (seed >> 15u);
	float value = float(seed) / (4294967296.0);
	return value;
}

struct CascadeInfo
{
    mat4 viewProj;
    vec4 frustumPlanes[6];
    vec4 cascadeScale;
};

// OpenGL core profile specs, section 8.13.
// Get 3d sampling vector from uv, useful when do cubemap filter on compute shader. 
vec3 getSamplingPosition(uint faceId, vec2 st)
{
    vec2 uv = 2.0 * vec2(st.x, 1.0 - st.y) - vec2(1.0);
    vec3 ret;

         if(faceId == 0) ret = vec3(  1.0,  uv.y, -uv.x);
    else if(faceId == 1) ret = vec3( -1.0,  uv.y,  uv.x);
    else if(faceId == 2) ret = vec3( uv.x,  1.0,  -uv.y);
    else if(faceId == 3) ret = vec3( uv.x, -1.0,   uv.y);
    else if(faceId == 4) ret = vec3( uv.x,  uv.y,   1.0);
    else if(faceId == 5) ret = vec3(-uv.x,  uv.y,  -1.0);

    return ret;
}

vec3 getSamplingVector(uint faceId, vec2 st)
{
    return normalize(getSamplingPosition(faceId, st));
}

struct DispatchIndirectCommand 
{
    uint x;
    uint y;
    uint z;
    uint pad;
};

// Build one TBN matrix from normal input.
// 
mat3 createTBN(vec3 N) 
{
    vec3 U;
    if (abs(N.z) > 0.0) 
    {
        float k = sqrt(N.y * N.y + N.z * N.z);
        U.x = 0.0; 
        U.y = -N.z / k; 
        U.z = N.y / k;
    }
    else 
    {
        float k = sqrt(N.x * N.x + N.y * N.y);
        U.x = N.y / k; 
        U.y = -N.x / k; 
        U.z = 0.0;
    }

    mat3 TBN = mat3(U, cross(N, U), N);
    return transpose(TBN);
}

#endif