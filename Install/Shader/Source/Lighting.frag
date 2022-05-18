#version 460

layout (location = 0) in  vec2 inUV0;
layout (location = 0) out vec4 outHdrSceneColor;

#define FRAME_DATA_SET 0
#include "FrameData.glsl"

#define VIEW_DATA_SET 1
#include "ViewData.glsl"

#include "Common.glsl"
#include "Sampler.glsl"
#include "AtmosphereUtils.glsl"

layout(set = 2, binding = 0) readonly buffer CascadeInfoBuffer
{
	CascadeInfo cascadeInfosbuffer[];
};

// GBuffer A: r8g8b8a8 unorm, .rgb store base color
layout(set = 3, binding = 0) uniform sampler2D inGBufferA;

// GBuffer B: r16g16b16a16 sfloat, .rgb store worldspace normal, .a is shading model id.
layout(set = 3, binding = 1) uniform sampler2D inGBufferB;

// GBuffer C: r16g16b16a16 sfloat, .rgb store emissive color
layout(set = 3, binding = 2) uniform sampler2D inGBufferC;

// GBuffer S: r8g8b8a8 unorm, .r is metal, .g is roughness, .b is mesh ao.
layout(set = 3, binding = 3) uniform sampler2D inGBufferS;

// Device depth sampler.
layout(set = 3, binding = 4) uniform sampler2D inDepth;

// sdsm shadow altas.
layout(set = 3, binding = 5) uniform sampler2DShadow inShadow; 

// BRDF Lut.
layout(set = 3, binding = 6) uniform sampler2D inBRDFLut;

layout(set = 3, binding = 7) uniform samplerCube inEnvSpecularPrefilter;

layout(set = 3, binding = 8) uniform sampler2D inSSR;

layout(set = 3, binding = 9) uniform sampler2D inSSAO;

layout(set = 3, binding = 10) uniform sampler2D inTransimition;

struct PushConstantsData
{
    uint cascadeCount;
    uint shadowMapPageDimXY;

    uint validAtmosphere;
    float groundRadiusMM;
    float atmosphereRadiusMM;
    float offsetHeight;
};

layout(push_constant) uniform constants
{   
   PushConstantsData pushConstant;
};

#include "SDSMLightingCommon.glsl"
#include "BRDF.glsl"

// Activision GTAO paper: https://www.activision.com/cdn/research/s2016_pbs_activision_occlusion.pptx
vec3 GTAOMultiBounce(float AO, vec3 baseColor)
{
    vec3 a =  2.0404 * baseColor - 0.3324;
    vec3 b = -4.7951 * baseColor + 0.6417;
    vec3 c =  2.7552 * baseColor + 0.6903;

    vec3 x  = vec3(AO);

    return max(x, ((x * a + b) * x + c) * x);
}

GbufferData loadGbufferData()
{
    GbufferData gData;

    vec2 renderSize = textureSize(inGBufferA, 0).xy;
    vec2 texelSize = 1.0f / renderSize;

    vec4 gbufferA = texture(inGBufferA, inUV0);
    vec4 gbufferB = texture(inGBufferB, inUV0);
    vec4 gbufferC = texture(inGBufferC, inUV0);
    vec4 gbufferS = texture(inGBufferS, inUV0);

    gData.baseColor = gbufferA.xyz;
    gData.worldNormal = normalize(gbufferB.xyz);
    gData.shadingModelId = gbufferB.w;
    gData.emissiveColor = gbufferC.xyz;

    vec2 ssaoTex = texture(inSSAO, inUV0).rg;

    gData.so = ssaoTex.y;
    gData.ao = GTAOMultiBounce(min(gbufferS.b, ssaoTex.r), gData.baseColor);

    gData.metal = gbufferS.r;
    gData.roughness = gbufferS.g;
    gData.F0 = mix(vec3(0.04f), gData.baseColor, vec3(gData.metal));

    float sampleDeviceDepth = texture(inDepth,inUV0).r;
    gData.deviceDepth = clamp(sampleDeviceDepth,0.0f,1.0f);

    vec4 posClip  = vec4(inUV0.x * 2.0f - 1.0f, 1.0f - inUV0.y * 2.0f, gData.deviceDepth, 1.0f);
    vec4 posWorldRebuild = viewData.camInvertViewProjectionJitter * posClip;
    gData.worldPos = posWorldRebuild.xyz / posWorldRebuild.w;

    return gData;
}

void main()
{
    GbufferData gData = loadGbufferData();
    if(matchShadingModel(gData.shadingModelId, SHADING_MODEL_UNVALID))
    {
        discard;
    }

    vec2 rtSize = vec2(textureSize(inDepth, 0));

    vec3 worldPos = gData.worldPos;
    vec3 viewPos  = (viewData.camView * vec4(worldPos, 1.0f)).xyz;

    vec3 directionalLightColor = frameData.directionalLightColor.xyz * 3.14;

    vec3 L = normalize(-frameData.directionalLightDir.xyz);
    vec3 N = gData.worldNormal;
    vec3 V = normalize(viewData.camWorldPos.xyz - worldPos);
    vec3 H = normalize(V + L);

    float NoL = max(0.0f,dot(N, L));
    float LoH = max(0.0f,dot(L, H));
    float VoH = max(0.0f,dot(V, H));
    float NoV = max(0.0f,dot(N, V));
    float NoH = max(0.0f,dot(N, H));

    // reflect vector.
    vec3 Lr   = reflect(-V, N);
    vec3 F0   = gData.F0; 

    uint cascadeId;
    float directShadow = directShadowEvaluate(worldPos, NoL, N, cascadeId, inUV0 * rtSize, inUV0);

    const vec3 ambientLight = vec3(0.35f);
    float screenFadeFactor = screenFade(inUV0);

    if(matchShadingModel(gData.shadingModelId, SHADING_MODEL_DisneyBRDF))
    {
        // atmosphere trans
        vec3 trans = vec3(1.0f);
        if(pushConstant.validAtmosphere > 0)
        {
            trans = getValFromTLUT(
                inTransimition,
                getViewPos(pushConstant.groundRadiusMM, pushConstant.offsetHeight, worldPos),
                L, 
                pushConstant.groundRadiusMM, 
                pushConstant.atmosphereRadiusMM
            );

            // QUESTION: Really need to do gamma inverse here?

            // NOTE: epic sebh's atmosphere model transmittance base on Bruneton08.
            //       it need to do a reverse gamma to ensure final color is linear space.

            // on sebh's example project, he just all sample in gamma space, final tonemapper do a final gamma correct.
            // if don't do this process, atmosphere result will very yellow.
            
            // i can't decide whether adding this process or not.
            trans = pow(trans, vec3(1.0f / 2.2f));
        }
        

        // direct lighting.
        vec3 directLight = vec3(0.0f);
        {
            vec3  F   = FresnelSchlick(VoH, F0);
            float D   = DistributionGGX(NoH, gData.roughness);   
            float G   = GeometrySmith(NoV, NoL, gData.roughness);      
            vec3 kD = mix(vec3(1.0) - F, vec3(0.0), gData.metal);

            vec3 diffuseBRDF  = kD * gData.baseColor;
            vec3 specularBRDF = (F * D * G) / max(0.00001, 4.0 * NoL * NoV);
        
            directLight = (diffuseBRDF + specularBRDF) * NoL;
        } 

        vec3 ambinetLighting = vec3(0.0f);
        {
            vec3 irradiance = ambientLight;

		    vec3 F = FresnelSchlick(NoV,F0);
		    vec3 kd = mix(vec3(1.0) - F, vec3(0.0), gData.metal);
		    ambinetLighting = kd * gData.baseColor * irradiance;
        }

        vec3 visibilityTerm =  vec3(directShadow) + ambinetLighting;

        vec3 reflectionColor;
        {
            int specularTextureLevels = textureQueryLevels(inEnvSpecularPrefilter);
            vec3 Lr   = reflect(-V, N);
            vec3 specularIrradiance = textureLod(inEnvSpecularPrefilter, Lr, gData.roughness * specularTextureLevels).rgb;

            vec4 ssrColor = texture(inSSR, inUV0);

            float factorSSR = ssrColor.w * screenFadeFactor;
            specularIrradiance = mix(specularIrradiance, ssrColor.xyz, factorSSR);

            vec2 specularBRDF = texture(inBRDFLut, vec2(NoV, gData.roughness)).rg;

            reflectionColor = (F0 * specularBRDF.x + specularBRDF.y) * specularIrradiance;
        }

        vec3 diffuseTerm = (directionalLightColor * (directLight * visibilityTerm) + ambinetLighting) * gData.ao * trans;
        vec3 specularTerm = reflectionColor * gData.so;

        outHdrSceneColor.rgb = diffuseTerm + specularTerm + 3.14f * gData.emissiveColor;
    }

    outHdrSceneColor.a   = 1.0f;
}