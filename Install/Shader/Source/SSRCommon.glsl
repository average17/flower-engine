#ifndef SSR_COMMON_GLSL
#define SSR_COMMON_GLSL

// simple Stochastic Screen Space Reflections implement.
// Resolve and temporal pass highly reference from https://github.com/MaxwellGengYF/Unity-MPipeline.

// Pipeline:
//      0. Full res 1 ray screen space reflection trace. 
//      1. Full res 4 tap resolve, radius scale by roughness.
//      2. Full res temporal accumulate, avoid main temporal AA variance clip ssr contribution and cause noise.

// denoise quality is no good. maybe i should try some denoise algorithm like AMD's SSSR.

#include "Common.glsl"

#define FRAME_DATA_SET 0
#include "FrameData.glsl"

#define VIEW_DATA_SET 1
#include "ViewData.glsl"

// ssr hit color and importance sample pdf.
layout (set = 2, binding = 0) uniform sampler2D inSceneColor; // History image, use for multi-boundce.

layout (set = 2, binding = 1) uniform sampler2D inGBufferS; // GBuffer S: r8g8b8a8 unorm, .r is metal, .g is roughness, .b is mesh ao.
layout (set = 2, binding = 2) uniform sampler2D inGBufferB; // GBuffer B: r16g16b16a16 sfloat, .rgb store worldspace normal, .a is shading model id
layout (set = 2, binding = 3) uniform sampler2D inHiz;
layout (set = 2, binding = 4) uniform sampler2D inBlueNoise1Spp;

layout (set = 2, binding = 5, rgba16f) uniform image2D SSRReflection;
layout (set = 2, binding = 6, rgba16f) uniform image2D SSRReflectionPrev;
layout (set = 2, binding = 7) uniform sampler2D inSSRReflection;
layout (set = 2, binding = 8) uniform sampler2D inSSRReflectionPrev;

layout (set = 2, binding = 9) uniform sampler2D inVelocity;

layout (set = 2, binding = 10) uniform sampler2D inTemporalFilter;
layout (set = 2, binding = 11, rgba16f) uniform image2D outTemporalFilter;

layout (set = 2, binding = 12) uniform sampler2D inGBufferA;

layout (set = 2, binding = 13, rgba16f) uniform image2D SSRIntersect;
layout (set = 2, binding = 14, r8) uniform image2D SSRHitMask;
layout (set = 2, binding = 15) uniform sampler2D inSSRIntersect;
layout (set = 2, binding = 16) uniform sampler2D inSSRHitMask;

layout (set = 2, binding = 17) uniform sampler2D inSceneDepth;

#define BLUE_NOISE_SET 3
#include "BlueNoiseCommon.glsl"

struct PushConstants
{
    uvec2 dimXY; // .xy is full res width and height.
    float blueNoiseDimX;
    float blueNoiseDimY;
    float thickness; // ssr test thickness.
    float cut; // 0.0f is cut, 1.0f is off.
    float maxRoughness;
	float temporalWeight;
	float temporalClampScale;
};

layout(push_constant) uniform constants
{   
    PushConstants pushConstant;
};

vec2 getBlueNoiseRandom1Spp(uvec2 pixelPos)
{
    return texelFetch(inBlueNoise1Spp,ivec2(pixelPos.xy % 128), 0).rg;
}

vec2 getBlueNoiseRandom16Spp(uvec2 pixelPos, uint index)
{
    uvec2 safePixelPos = pixelPos.xy % 128;

    float taaOffset = interleavedGradientNoiseTAA(safePixelPos, frameData.frameIndex.z, 16);
    float taaAngle  = taaOffset * 3.14159265359 * 2.0f;

    float s = sin(taaAngle);
    float c = cos(taaAngle);

    vec2 u = vec2(
        SampleRandomNumber16Spp(safePixelPos.x, safePixelPos.y, index, 0u),
        SampleRandomNumber16Spp(safePixelPos.x, safePixelPos.y, index, 1u)
    );

    u = vec2(u.x * c  + u.y * s, u.x * -s + u.y * c); 

    return u;
}

vec2 getBlueNoiseRandom4Spp(uvec2 pixelPos, uint index)
{
    uvec2 safePixelPos = pixelPos.xy % 128;

    float taaOffset = interleavedGradientNoiseTAA(safePixelPos, frameData.frameIndex.z, 16);
    float taaAngle  = taaOffset * 3.14159265359 * 2.0f;

    float s = sin(taaAngle);
    float c = cos(taaAngle);

    vec2 u = vec2(
        SampleRandomNumber4Spp(safePixelPos.x, safePixelPos.y, index, 0u),
        SampleRandomNumber4Spp(safePixelPos.x, safePixelPos.y, index, 1u)
    );

    u = vec2(u.x * c  + u.y * s, u.x * -s + u.y * c); 

    return u;
}

///////////////////////////////////////////////////////////////////////
// hiz trace.
vec2 SSRGetMipResolution(vec2 screenDimensions, int mipLevel) 
{
    return screenDimensions * pow(0.5, mipLevel);
}

void SSRInitialAdvanceRay(
    vec3 origin, 
    vec3 direction, 
    vec3 invDirection, 
    vec2 currentMipResolution, 
    vec2 currentMipResolutionInv, 
    vec2 floorOffset, 
    vec2 uvOffset, 
    out vec3 position, 
    out float currentT) 
{
    vec2 currentMipPosition = currentMipResolution * origin.xy;

    // Intersect ray with the half box that is pointing away from the ray origin.
    vec2 xyPlane = floor(currentMipPosition) + floorOffset;
    xyPlane = xyPlane * currentMipResolutionInv + uvOffset;

    // o + d * t = p' => t = (p' - o) / d
    vec2 t = xyPlane * invDirection.xy - origin.xy * invDirection.xy;
    currentT = min(t.x, t.y);
    position = origin + currentT * direction;
}

bool SSRAdvanceRay(
    vec3 origin, 
    vec3 direction, 
    vec3 invDirection, 
    vec2 currentMipPosition, 
    vec2 currentMipResolutionInv, 
    vec2 floorOffset, 
    vec2 uvOffset, 
    float surfaceZ, 
    inout vec3 position, 
    inout float currentT) 
{
    // Create boundary planes
    vec2 xyPlane = floor(currentMipPosition) + floorOffset;
    xyPlane = xyPlane * currentMipResolutionInv + uvOffset;
    vec3 boundaryPlanes = vec3(xyPlane, surfaceZ);

    // Intersect ray with the half box that is pointing away from the ray origin.
    // o + d * t = p' => t = (p' - o) / d
    vec3 t = boundaryPlanes * invDirection - origin * invDirection;

    // reverse z                                          // no reverse z.
    t.z = direction.z < 0 ? t.z : 3.402823466e+38;        // t.z = direction.z > 0 ? t.z : 3.402823466e+38;

    // Choose nearest intersection with a boundary.
    float tMin = min(min(t.x, t.y), t.z);

    // reverse z so larger z means closer to the camera.  // no reverse z.
    bool bAboveSurface = surfaceZ < position.z;           // bool bAboveSurface = surfaceZ > position.z;
    
    // Decide whether we are able to advance the ray until we hit the xy boundaries or if we had to clamp it at the surface.
    // We use the floatBitsToUint comparison to avoid NaN / Inf logic, also we actually care about bitwise equality here to see if tMin is the t.z we fed into the min3 above.
    bool bSkippedTile = floatBitsToUint(tMin) != floatBitsToUint(t.z) && bAboveSurface; 

    // Make sure to only advance the ray if we're still above the surface.
    currentT = bAboveSurface ? tMin : currentT;

    // Advance ray
    position = origin + currentT * direction;

    return bSkippedTile;
}

float SSRLoadDepth(ivec2 pixelCoordinate, int mip) 
{
    // we use max z, which is nearest depth on reverse z.
    // so we can find intersection if ray.z is behind nearest z.
    // if behind, find intersect and march to more detail level.
    return texelFetch(inHiz,pixelCoordinate, mip).g;
}

vec3 SSRHizTrace(
    vec3 origin, 
    vec3 direction, 
    vec2 screenSize, 
    int mostDetailedMip, 
    uint maxTraversalIntersections, 
    out bool bValidHit)
{
    // Start on mip with highest detail.
    int currentMip = mostDetailedMip;

    // Could recompute these every iteration, but it's faster to hoist them out and update them.
    vec2 currentMipResolution = SSRGetMipResolution(screenSize, currentMip);
    vec2 currentMipResolutionInv = 1.0f / currentMipResolution;

    // Offset to the bounding boxes uv space to intersect the ray with the center of the next pixel.
    // This means we ever so slightly over shoot into the next region. 
    vec2 uvOffset = 0.005 * exp2(mostDetailedMip) / screenSize;
    uvOffset.x = direction.x < 0 ? -uvOffset.x : uvOffset.x;
    uvOffset.y = direction.y < 0 ? -uvOffset.y : uvOffset.y;

    // Offset applied depending on current mip resolution to move the boundary to the left/right upper/lower border depending on ray direction.
    vec2 floorOffset;
    floorOffset.x = direction.x < 0 ? 0 : 1;
    floorOffset.y = direction.y < 0 ? 0 : 1;
    
    // Initially advance ray to avoid immediate self intersections.
    float currentT;
    vec3 position;

    vec3 invDirection;
    invDirection.x = direction.x != 0 ? 1.0 / direction.x : 3.402823466e+38;
    invDirection.y = direction.y != 0 ? 1.0 / direction.y : 3.402823466e+38;
    invDirection.z = direction.z != 0 ? 1.0 / direction.z : 3.402823466e+38;
    SSRInitialAdvanceRay(origin, direction, invDirection, currentMipResolution, currentMipResolutionInv, floorOffset, uvOffset, position, currentT);

    int i = 0;
    while (i < maxTraversalIntersections && currentMip >= mostDetailedMip) 
    {
        vec2 currentMipPosition = currentMipResolution * position.xy;

        float surfaceZ = SSRLoadDepth(ivec2(currentMipPosition), currentMip);

        bool bSkippedTile = SSRAdvanceRay(origin, direction, invDirection, currentMipPosition, currentMipResolutionInv, floorOffset, uvOffset, surfaceZ, position, currentT);

        currentMip += bSkippedTile ? 1 : -1;
        currentMipResolution *= bSkippedTile ? 0.5 : 2;
        currentMipResolutionInv *= bSkippedTile ? 2 : 0.5;

        ++i;
    }

    bValidHit = (i <= maxTraversalIntersections);

    return position;
}

vec3 SSRInvProjectPosition(vec3 coord, mat4 mat)
{
    coord.y  = (1.0f - coord.y);
    coord.xy = 2.0f * coord.xy - 1.0f;

    vec4 projected = mat * vec4(coord, 1.0f);
    projected.xyz /= projected.w;

    return projected.xyz;
}

vec3 SSRProjectPosition(vec3 origin, mat4 mat) 
{
    vec4 projected = mat * vec4(origin, 1);

    projected.xyz /= projected.w;
    projected.xy = 0.5 * projected.xy + 0.5;
    projected.y = (1 - projected.y);

    return projected.xyz;
}

vec2 getMotionVector(float sceneDepth, vec2 inUV)
{
    vec4 posClip  = vec4(inUV.x * 2.0f - 1.0f, 1.0f - inUV.y * 2.0f, sceneDepth, 1.0f);
    vec4 posWorldRebuild = viewData.camInvertViewProjectionJitter * posClip;

    vec4 worldPos = vec4(posWorldRebuild.xyz / posWorldRebuild.w, 1.0f);

    vec4 curClipPos =  viewData.camViewProj * worldPos;
    vec4 prevClipPos =  viewData.camViewProjLast * worldPos;

    curClipPos.xy /= curClipPos.w;
    prevClipPos.xy /= prevClipPos.w;

    vec2 v = (curClipPos.xy - prevClipPos.xy) * 0.5f;
    v.y *= -1.0f;

    return v;
}

float SSRValidateHit(
    vec3 hit, 
    vec2 uv, 
    vec3 worldSpaceRayDirection, 
    vec2 screenSize, 
    float depthBufferThickness,
    float roughness) 
{
    // Reject hits outside the view frustum
    if((hit.x < 0) || (hit.y < 0) || (hit.x > 1) || (hit.y > 1))
    {
        return 0;
    }

    // Reject the hit if we didnt advance the ray significantly to avoid immediate self reflection
    vec2 manhattanDist = abs(hit.xy - uv);
    vec2 manhattanDistEdge = 2.0f / screenSize;
    if((manhattanDist.x < manhattanDistEdge.x) && (manhattanDist.y < manhattanDistEdge.y)) 
    {
        return 0;
    }

    // Don't lookup radiance from the background.
    ivec2 texelCoords = ivec2(screenSize * hit.xy);
    float surfaceZ = SSRLoadDepth(texelCoords / 2, 1);

    // reverse z         // no reverse z
    if (surfaceZ == 0.0) // == 1.0
    {
        return 0;
    }

    // current close, because we want more reflection detail for foliage and other single face mesh.
    // We check if we hit the surface from the back, these should be rejected.
    float hitBackFactor = 1.0f;
    vec3 hitNormal = normalize(texelFetch(inGBufferB, texelCoords, 0).rgb);
    if (dot(hitNormal, worldSpaceRayDirection) > 0) 
    {
        // hit back face ? mix by roughness. mirror plane will give up, roughness plane will keep.
        hitBackFactor = mix(0.0f, 1.0f, clamp(roughness * 4.0f, 0.0f, 1.0f));
    }

    vec3 viewSpaceSurface = SSRInvProjectPosition(vec3(hit.xy, surfaceZ), inverse(viewData.camProjJitter));
    vec3 viewSpaceHit = SSRInvProjectPosition(hit, inverse(viewData.camProjJitter));
    float distance = length(viewSpaceSurface - viewSpaceHit);

    // Fade out hits near the screen borders
    vec2 fov = 0.05 * vec2(screenSize.y / screenSize.x, 1);
    vec2 border = smoothstep(vec2(0), fov, hit.xy) * (1 - smoothstep(vec2(1 - fov), vec2(1), hit.xy));
    float vignette = border.x * border.y;

    // We accept all hits that are within a reasonable minimum distance below the surface.
    // Add constant in linear space to avoid growing of the reflections toward the reflected objects.
    float confidence = 1 - smoothstep(0, depthBufferThickness, distance);
    confidence *= confidence;

    return vignette * confidence * hitBackFactor;
}

bool isMirrorReflection(float roughness) 
{
    return roughness < 0.01;
}

vec4 TangentToWorld(vec3 N, vec4 H)
{
	vec3 UpVector = abs(N.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
	vec3 T = normalize(cross(UpVector, N));
	vec3 B = cross(N, T);

	return vec4((T * H.x) + (B * H.y) + (N * H.z), H.w);
}

mat3 CreateTBN(vec3 N) {
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

    mat3 TBN;
    TBN[0] = U;
    TBN[1] = cross(N, U);
    TBN[2] = N;
    return transpose(TBN);
}

// http://jcgt.org/published/0007/04/01/paper.pdf by Eric Heitz
// Input Ve: view direction
// Input alphaX, alphaY: roughness parameters
// Input U1, U2: uniform random numbers
// Output Ne: normal sampled with PDF D_Ve(Ne) = G1(Ve) * max(0, dot(Ve, Ne)) * D(Ne) / Ve.z
vec3 SampleGGXVNDF(vec3 Ve, float alphaX, float alphaY, float U1, float U2) 
{
    // Section 3.2: transforming the view direction to the hemisphere configuration
    vec3 Vh = normalize(vec3(alphaX * Ve.x, alphaY * Ve.y, Ve.z));

    // Section 4.1: orthonormal basis (with special case if cross product is zero)
    float lensq = Vh.x * Vh.x + Vh.y * Vh.y;
    vec3 T1 = lensq > 0 ? vec3(-Vh.y, Vh.x, 0) / sqrt(lensq) : vec3(1, 0, 0);
    vec3 T2 = cross(Vh, T1);

    // Section 4.2: parameterization of the projected area
    float r = sqrt(U1);
    float phi = 2.0 * 3.14159265359 * U2;
    float t1 = r * cos(phi);
    float t2 = r * sin(phi);
    float s = 0.5 * (1.0 + Vh.z);
    t2 = (1.0 - s) * sqrt(1.0 - t1 * t1) + s * t2;

    // Section 4.3: reprojection onto hemisphere
    vec3 Nh = t1 * T1 + t2 * T2 + sqrt(max(0.0, 1.0 - t1 * t1 - t2 * t2)) * Vh;

    // Section 3.4: transforming the normal back to the ellipsoid configuration
    vec3 Ne = normalize(vec3(alphaX * Nh.x, alphaY * Nh.y, max(0.0, Nh.z)));

    return Ne;
}

vec3 SampleReflectionVector(vec3 viewDirection, vec3 normal, float roughness, vec2 u) 
{
    mat3 tbnTransform = CreateTBN(normal);
    vec3 viewDirectionTbn = tbnTransform * (-viewDirection);

    vec3 sampledNormalTbn = SampleGGXVNDF(viewDirectionTbn,roughness, roughness, u.x, u.y);
    vec3 reflectedDirectionTbn = reflect(-viewDirectionTbn, sampledNormalTbn);

    // Transform reflected_direction back to the initial space.
    mat3 invTbnTransform = transpose(tbnTransform);
    return invTbnTransform * reflectedDirectionTbn;
}

#endif