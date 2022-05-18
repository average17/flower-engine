#ifndef SDSMLIGHTING_COMMON_GLSL
#define SDSMLIGHTING_COMMON_GLSL

vec3 biasNormalOffset(vec3 N, float NoL, float texelSize)
{
    return N * clamp(1.0f - NoL, 0.0f, 1.0f) * texelSize * 10.0f * 3.0f;
}

float autoBias(float NoL, float biasMul)
{
    return 6e-4f + (1.0f - NoL) * biasMul * 1e-4f;
}

// Maybe blue noise is better.
const vec2 poisson_disk_25[25] = vec2[](
    vec2(-0.978698, -0.0884121),
    vec2(-0.841121, 0.521165),
    vec2(-0.71746, -0.50322),
    vec2(-0.702933, 0.903134),
    vec2(-0.663198, 0.15482),
    vec2(-0.495102, -0.232887),
    vec2(-0.364238, -0.961791),
    vec2(-0.345866, -0.564379),
    vec2(-0.325663, 0.64037),
    vec2(-0.182714, 0.321329),
    vec2(-0.142613, -0.0227363),
    vec2(-0.0564287, -0.36729),
    vec2(-0.0185858, 0.918882),
    vec2(0.0381787, -0.728996),
    vec2(0.16599, 0.093112),
    vec2(0.253639, 0.719535),
    vec2(0.369549, -0.655019),
    vec2(0.423627, 0.429975),
    vec2(0.530747, -0.364971),
    vec2(0.566027, -0.940489),
    vec2(0.639332, 0.0284127),
    vec2(0.652089, 0.669668),
    vec2(0.773797, 0.345012),
    vec2(0.968871, 0.840449),
    vec2(0.991882, -0.657338)
);

float shadowPcf(vec3 shadowCoord, vec2 texelSize, uint cascadeId, float perCascadeEdge, vec2 screenPos, vec2 screenUV)
{
    float shadow = 0.0f;
    float compareDepth = shadowCoord.z;

    float shadowFilterSize = 2.0f;
    uint activeShadowCouont = 0;

    const uint shadowSampleCount = 25;
    
    float taaOffset = interleavedGradientNoiseTAA(screenPos, frameData.frameIndex.z, 16);
    float taaAngle  = taaOffset * 3.14159265359 * 2.0f;

    for (uint i = 0; i < shadowSampleCount; i++)
    {
        float s = sin(taaAngle);
        float c = cos(taaAngle);

        vec2 offsetUv = vec2(
            poisson_disk_25[i].x * c  + poisson_disk_25[i].y * s, 
            poisson_disk_25[i].x * -s + poisson_disk_25[i].y * c); 
        
        offsetUv *= texelSize * shadowFilterSize;

        vec2 sampleUv = shadowCoord.xy + offsetUv;
        if(onRange(sampleUv.x, perCascadeEdge * cascadeId, perCascadeEdge * (cascadeId + 1)))
        {
            vec3 compareUv = vec3(sampleUv, compareDepth);
            shadow += texture(inShadow, compareUv);

            activeShadowCouont ++;
        }
    }
    
    if(activeShadowCouont != 0)
    {
        shadow /= float(activeShadowCouont);
    }
    else
    {
        shadow = 0.0f;
    }
    
    return shadow;
}

float directShadowEvaluate(vec3 worldPos, float safeNoL, vec3 N, uint cacsadeIdIn, vec2 screenPos, vec2 screenUV)
{
    const float cascadeBorderAdopt = 0.006f;
    const float shadowTexelSize = 1.0f / float(pushConstant.shadowMapPageDimXY);
    const float perCascadeOffsetUV = 1.0f / pushConstant.cascadeCount;

    // find active cascade.
    uint activeCascadeId = 0;
    vec3 shadowNdcPos;
    for(uint cascadeId = 0; cascadeId < pushConstant.cascadeCount; cascadeId ++)
    {
        vec4 shadowProjPos =  cascadeInfosbuffer[cascadeId].cascadeViewProjMatrix * vec4(worldPos, 1.0f);
        shadowNdcPos = shadowProjPos.xyz /= shadowProjPos.w;
        vec3 shadowCoord = vec3(shadowNdcPos.xy * 0.5f + 0.5f, shadowNdcPos.z);
        shadowCoord.y = 1.0f - shadowCoord.y;
        if(onRange(shadowCoord.xyz, vec3(cascadeBorderAdopt), vec3(1.0f - cascadeBorderAdopt)))
        {
            break;
        }
        activeCascadeId ++;
    }

    cacsadeIdIn = activeCascadeId;

    float shadowResult = 1.0f;
    vec3 offsetPos = biasNormalOffset(N, safeNoL, shadowTexelSize);
    {
        vec4 OffsetProjPos = cascadeInfosbuffer[activeCascadeId].cascadeViewProjMatrix * vec4(worldPos + offsetPos, 1.0f); 
        float shadowDepthBias = autoBias(safeNoL, activeCascadeId + 1);

        vec3 offsetNDCPos = OffsetProjPos.xyz / OffsetProjPos.w;
        offsetNDCPos.xy = offsetNDCPos.xy * 0.5f + 0.5f;
        offsetNDCPos.y = 1.0f - offsetNDCPos.y;
        vec3 shadowPosOnAltas = offsetNDCPos.xyz;
        shadowPosOnAltas.x = (shadowPosOnAltas.x + float(activeCascadeId)) * perCascadeOffsetUV;

        shadowPosOnAltas.z +=  shadowDepthBias;

        shadowResult = shadowPcf(shadowPosOnAltas, vec2(shadowTexelSize), activeCascadeId, perCascadeOffsetUV, screenPos, screenUV);
    }

    const float cascadeEdgeLerpThreshold = 0.8f;
    vec2 ndcPosAbs = abs(shadowNdcPos.xy);
    float cascadeFadeEdge = (max(ndcPosAbs.x, ndcPosAbs.y) - cascadeEdgeLerpThreshold) * 4.0f;
    if(cascadeFadeEdge > 0.0f && activeCascadeId < pushConstant.cascadeCount - 1)
    {
        uint lerpCascadeId = activeCascadeId + 1;
        vec4 lerpShadowProjPos = cascadeInfosbuffer[lerpCascadeId].cascadeViewProjMatrix * vec4(worldPos + offsetPos, 1.0f); 
        float shadowDepthBias = autoBias(safeNoL, lerpCascadeId + 1);

        lerpShadowProjPos.xyz /= lerpShadowProjPos.w;
        lerpShadowProjPos.xy = lerpShadowProjPos.xy * 0.5f + 0.5f;
        lerpShadowProjPos.y = 1.0f - lerpShadowProjPos.y;
        lerpShadowProjPos.x = (lerpShadowProjPos.x + float(lerpCascadeId)) * perCascadeOffsetUV;

        

        lerpShadowProjPos.z +=  shadowDepthBias;

        float lerpShadowValue = shadowPcf(lerpShadowProjPos.xyz, vec2(shadowTexelSize),  lerpCascadeId, perCascadeOffsetUV, screenPos, screenUV);

        shadowResult = mix(shadowResult, lerpShadowValue, cascadeFadeEdge);
    }

    return shadowResult;
}

#endif