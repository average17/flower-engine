#ifndef ATMOSPHERE_UE4_COMMON_GLSL
#define ATMOSPHERE_UE4_COMMON_GLSL

// my personal atmosphere implement from: A Scalable and Production Ready Sky and Atmosphere Rendering.
// also see https://github.com/sebh/UnrealEngineSkyAtmosphere if u just want a hlsl demo. XD.

// there are 
//  #pass 0: compute transmittance.
//  #pass 1: compute multi-scattering. 
//  #pass 2: compute skyview.
//  #pass 3: compute area-perspective. // NOIE: We relpace area-perspective with volumetric scattering, so don't compute here agian.
//  #pass 4: compute clear-sky.

#include "AtmosphereUtils.glsl"

#ifndef COMPUTE_3D
layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
#else
layout (local_size_x = 8, local_size_y = 8, local_size_z = 8) in;
#endif

#define FRAME_DATA_SET 0
#include "FrameData.glsl"

#define VIEW_DATA_SET 1
#include "ViewData.glsl"

#include "Common.glsl"

layout (set = 2, binding = 0, rgba16f) uniform image2D outTransmittanceLut;
layout (set = 2, binding = 1) uniform sampler2D inTransmittanceLut;

layout (set = 2, binding = 2, rgba16f) uniform image2D outMultipleScatteringLut;
layout (set = 2, binding = 3) uniform sampler2D inMultipleScatteringLut;

layout (set = 2, binding = 4, rgba16f) uniform image2D outSkyViewLut;
layout (set = 2, binding = 5) uniform sampler2D inSkyLut;

layout (set = 2, binding = 6) uniform sampler2D inDepth;
layout (set = 2, binding = 7, rgba16f) uniform image2D outHdrColor;

// Max size 128 byte.
// sizeof float is 4 bytes.
// = 32 float
// = 8 4x float
struct AtmosphereParameters
{
    // 4x float
    vec3 groundAlbedo;
    float mieAbsorptionBase; // 1x

    // 4x float
    vec3 rayleighScatteringBase;
    float rayleighAbsorptionBase; // 2x
    
    // 4x float
    vec3 ozoneAbsorptionBase;
    float mieScatteringBase; // 3x

    // 4x float
    float groundRadiusMM;
    float atmosphereRadiusMM;
    float offsetHeight;
    float atmosphereExposure; // 4x

    // 4x float
    float rayleighAltitude; 
    float mieAltitude; 
    float ozoneAltitude; 
    float ozoneThickness;    // 5x
};

layout(push_constant) uniform constants
{   
    AtmosphereParameters pushConstant;
};

// Config ////////////////////////////////////
const float PI = 3.14159265358;
const float mulScattSteps = 20.0;
const int   sqrtSamples = 8;
const float sunTransmittanceSteps = 40.0;
const int   numScatteringSteps = 32;
/////////////////////////////////////////////

// get View pos fit to planet unit.
vec3 getViewPos()
{
    return getViewPos(pushConstant.groundRadiusMM, pushConstant.offsetHeight, viewData.camWorldPos.xyz);
}

vec3 getSunDir()
{
    return -normalize(frameData.directionalLightDir.xyz);
}

float getMiePhase(float cosTheta) 
{
    const float g = 0.8;
    const float scale = 3.0/(8.0*PI);
    
    float num = (1.0 - g * g) * (1.0 + cosTheta * cosTheta);
    float denom = (2.0 + g * g ) * pow((1.0 + g * g - 2.0 * g * cosTheta), 1.5);
    
    return scale*num/denom;
}

float getRayleighPhase(float cosTheta) 
{
    const float k = 3.0 / (16.0 * PI);
    return k * (1.0 + cosTheta*cosTheta);
}

void getScatteringValues(
	vec3 pos, 
	out vec3 rayleighScattering, 
    out float mieScattering,
    out vec3 extinction) 
{
    float altitudeKM = (length(pos)-pushConstant.groundRadiusMM) * 1000.0;

    float rayleighDensity = exp(-altitudeKM /pushConstant.rayleighAltitude);
    float mieDensity = exp(-altitudeKM / pushConstant.mieAltitude);
    
    rayleighScattering = pushConstant.rayleighScatteringBase * rayleighDensity;
    float rayleighAbsorption = pushConstant.rayleighAbsorptionBase * rayleighDensity;
    
    mieScattering = pushConstant.mieScatteringBase * mieDensity;
    float mieAbsorption = pushConstant.mieAbsorptionBase * mieDensity;
    
    vec3 ozoneAbsorption = pushConstant.ozoneAbsorptionBase * 
        max(0.0, 1.0 - abs(altitudeKM - pushConstant.ozoneAltitude) / (0.5f * pushConstant.ozoneThickness));
    
    extinction = rayleighScattering + rayleighAbsorption + mieScattering + mieAbsorption + ozoneAbsorption;
}

float safeacos(const float x) 
{
    return acos(clamp(x, -1.0, 1.0));
}

float rayIntersectSphere(vec3 ro, vec3 rd, float rad) 
{
    float b = dot(ro, rd);
    float c = dot(ro, ro) - rad*rad;

    if (c > 0.0f && b > 0.0) 
        return -1.0;

    float discr = b*b - c;
    if (discr < 0.0) 
        return -1.0;

    if (discr > b*b) 
        return (-b + sqrt(discr));

    return -b - sqrt(discr);
}

// load value from transmittance lut.
vec3 getValFromTLUT(sampler2D tex, vec3 pos, vec3 sunDir) 
{
    return getValFromTLUT(tex, pos, sunDir, pushConstant.groundRadiusMM, pushConstant.atmosphereRadiusMM);
}

// load value from multi-scatter lut.
vec3 getValFromMultiScattLUT(sampler2D tex, vec3 pos, vec3 sunDir) 
{
    float height = length(pos);
    vec3 up = pos / height;
	float sunCosZenithAngle = dot(sunDir, up);

    vec2 uv = vec2(
        clamp(0.5 + 0.5 * sunCosZenithAngle, 0.0, 1.0), 
        max(0.0, min(1.0, (height - pushConstant.groundRadiusMM) /
        (pushConstant.atmosphereRadiusMM - pushConstant.groundRadiusMM)))
    ); 

    return texture(tex, uv).rgb;
}

// load value from sky lut.
vec3 getValFromSkyLUT(sampler2D tex,vec3 rayDir, vec3 sunDir) 
{
    vec3 camViewPos = getViewPos();

    float height = length(camViewPos);
    vec3 up = camViewPos / height;
    
    float horizonAngle = safeacos(sqrt(height * height - pushConstant.groundRadiusMM * pushConstant.groundRadiusMM) / height);
    float altitudeAngle = horizonAngle - acos(dot(rayDir, up)); 
    float azimuthAngle; 

    if (abs(altitudeAngle) > (0.5*PI - .0001)) 
	{
        azimuthAngle = 0.0;
    } 
	else 
	{
        vec3 right = cross(sunDir, up);
        vec3 forward = cross(up, right);
        
        vec3 projectedDir = normalize(rayDir - up*(dot(rayDir, up)));
        float sinTheta = dot(projectedDir, right);
        float cosTheta = dot(projectedDir, forward);
        azimuthAngle = atan(sinTheta, cosTheta) + PI;
    }

    float v = 0.5 + 0.5 * sign(altitudeAngle)*sqrt(abs(altitudeAngle)*2.0/PI);
    vec2 uv = vec2(azimuthAngle / (2.0 * PI), v);
    
    return texture(tex, uv).rgb;
}

// compute spherical direction by theta and phi.
vec3 getSphericalDir(float theta, float phi) 
{
    float cosPhi = cos(phi);
    float sinPhi = sin(phi);
    float cosTheta = cos(theta);
    float sinTheta = sin(theta);

    return vec3(sinPhi*sinTheta, cosPhi, sinPhi*cosTheta);
}

void getMulScattValues(vec3 pos, vec3 sunDir, out vec3 lumTotal, out vec3 fms) 
{
    lumTotal = vec3(0.0);
    fms = vec3(0.0);
    
    float invSamples = 1.0/float(sqrtSamples*sqrtSamples);
    for (int i = 0; i < sqrtSamples; i++) 
    {
        for (int j = 0; j < sqrtSamples; j++) 
        {
            float theta = PI * (float(i) + 0.5) / float(sqrtSamples);
            float phi = safeacos(1.0 - 2.0*(float(j) + 0.5) / float(sqrtSamples));
            vec3 rayDir = getSphericalDir(theta, phi);
            
            float atmoDist = rayIntersectSphere(pos, rayDir, pushConstant.atmosphereRadiusMM);
            float groundDist = rayIntersectSphere(pos, rayDir, pushConstant.groundRadiusMM);
            float tMax = atmoDist;

            if (groundDist > 0.0) 
            {
                tMax = groundDist;
            }
            
            float cosTheta = dot(rayDir, sunDir);
    
            float miePhaseValue = getMiePhase(cosTheta);
            float rayleighPhaseValue = getRayleighPhase(-cosTheta);
            
            vec3 lum = vec3(0.0), lumFactor = vec3(0.0), transmittance = vec3(1.0);
            float t = 0.0;

            for (float stepI = 0.0; stepI < mulScattSteps; stepI += 1.0) 
            {
                float newT = ((stepI + 0.3)/mulScattSteps)*tMax;
                float dt = newT - t;
                t = newT;

                vec3 newPos = pos + t*rayDir;

                vec3 rayleighScattering, extinction;
                float mieScattering;
                getScatteringValues(newPos, rayleighScattering, mieScattering, extinction);

                vec3 sampleTransmittance = exp(-dt*extinction);
                
                vec3 scatteringNoPhase = rayleighScattering + mieScattering;
                vec3 scatteringF = (scatteringNoPhase - scatteringNoPhase * sampleTransmittance) / extinction;
                lumFactor += transmittance*scatteringF;
                
                vec3 sunTransmittance = getValFromTLUT(inTransmittanceLut, newPos, sunDir);

                vec3 rayleighInScattering = rayleighScattering*rayleighPhaseValue;
                float mieInScattering = mieScattering*miePhaseValue;
                vec3 inScattering = (rayleighInScattering + mieInScattering)*sunTransmittance;

                vec3 scatteringIntegral = (inScattering - inScattering * sampleTransmittance) / extinction;

                lum += scatteringIntegral*transmittance;
                transmittance *= sampleTransmittance;
            }
            
            if (groundDist > 0.0) 
            {
                vec3 hitPos = pos + groundDist*rayDir;
                if (dot(pos, sunDir) > 0.0) 
                {
                    hitPos = normalize(hitPos) * pushConstant.groundRadiusMM;
                    lum += transmittance * pushConstant.groundAlbedo * getValFromTLUT(inTransmittanceLut, hitPos, sunDir);
                }
            }
            
            fms += lumFactor*invSamples;
            lumTotal += lum*invSamples;
        }
    }
}

vec3 drawSun(vec3 rayDir, vec3 sunDir) 
{
    const float theta = 0.25 * PI / 180.0;
    const float cT = cos(theta);

    float dT = dot(rayDir, sunDir);

    if (dT >= cT) 
        return vec3(1.0);
    
    float oo = cT - dT;
    float bloom = exp(-oo * 50000.0f) * 0.5f;
    float blackHole = 1.0f / (0.02f + oo * 300.0f) * 0.01f;
    return vec3(bloom + blackHole);
}

vec3 raymarchScattering(vec3 pos, vec3 rayDir, vec3 sunDir, float tMax, float numSteps, out vec3 ts) 
{
    float cosTheta = dot(rayDir, sunDir);
    
	float miePhaseValue = getMiePhase(cosTheta);
	float rayleighPhaseValue = getRayleighPhase(-cosTheta);
    
    vec3 lum = vec3(0.0);
    vec3 transmittance = vec3(1.0);
    float t = 0.0;
    for (float i = 0.0; i < numSteps; i += 1.0) 
    {
        float newT = ((i + 0.3)/numSteps)*tMax;
        float dt = newT - t;
        t = newT;
        
        vec3 newPos = pos + t*rayDir;
        
        vec3 rayleighScattering, extinction;
        float mieScattering;
        getScatteringValues(newPos, rayleighScattering, mieScattering, extinction);
        
        vec3 sampleTransmittance = exp(-dt*extinction);

        vec3 sunTransmittance = getValFromTLUT(inTransmittanceLut, newPos, sunDir);
        vec3 psiMS = getValFromMultiScattLUT(inMultipleScatteringLut, newPos, sunDir);
        
        vec3 rayleighInScattering = rayleighScattering*(rayleighPhaseValue*sunTransmittance + psiMS);
        vec3 mieInScattering = mieScattering*(miePhaseValue*sunTransmittance + psiMS);
        vec3 inScattering = (rayleighInScattering + mieInScattering);

        vec3 scatteringIntegral = (inScattering - inScattering * sampleTransmittance) / extinction;

        lum += scatteringIntegral*transmittance;
        
        transmittance *= sampleTransmittance;
    }
    ts = transmittance;
    return lum;
}

vec3 getSunTransmittance(vec3 pos, vec3 sunDir) 
{
    if (rayIntersectSphere(pos, sunDir, pushConstant.groundRadiusMM) > 0.0) 
    {
        return vec3(0.0);
    }
    
    float atmoDist = rayIntersectSphere(pos, sunDir, pushConstant.atmosphereRadiusMM);
    float t = 0.0;
    
    vec3 transmittance = vec3(1.0);
    for (float i = 0.0; i < sunTransmittanceSteps; i += 1.0) 
    {
        float newT = ((i + 0.3)/sunTransmittanceSteps)*atmoDist;
        float dt = newT - t;
        t = newT;
        
        vec3 newPos = pos + t*sunDir;
        
        vec3 rayleighScattering, extinction;
        float mieScattering;
        getScatteringValues(newPos, rayleighScattering, mieScattering, extinction);
        
        transmittance *= exp(-dt*extinction);
    }
    return transmittance;
}

#endif