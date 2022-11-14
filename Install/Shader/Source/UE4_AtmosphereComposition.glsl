#version 460

#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_samplerless_texture_functions : enable

#include "UE4_AtmosphereCommon.glsl"

vec3 drawSun(vec3 rayDir, vec3 sunDir) 
{
    const float theta = 0.3 * kPI / 180.0;
    const vec3 sunCenterColor = vec3(1.0f, 0.92549, 0.87843) * 5000.0f;


    const float cT = cos(theta);
    float dT = dot(rayDir, sunDir);

    if (dT >= cT) 
    {
        return sunCenterColor;
    }
    return vec3(0.0);
}

#include "Schedule.glsl"

layout (local_size_x = 8, local_size_y = 8) in;
void main() 
{
    ivec2 colorSize = imageSize(imageHdrSceneColor);

    uvec2 groupThreadId = remap8x8(gl_LocalInvocationIndex);
    uvec2 dispatchId = groupThreadId + gl_WorkGroupID.xy * 8;

    ivec2 workPos = ivec2(dispatchId);

    if(workPos.x >= colorSize.x || workPos.y >= colorSize.y)
    {
        return;
    }

    const vec2 pixPos = vec2(workPos) + vec2(0.5f);
    const vec2 uv = pixPos / vec2(colorSize);

    AtmosphereParameters atmosphere = getAtmosphereParameters();

    // We are revert z.
    vec4 clipSpace = vec4(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f, 0.0, 1.0);
	vec4 viewPosH = viewData.camInvertProj * clipSpace;
    vec3 viewDir = viewPosH.xyz / viewPosH.w;
    vec3 worldDir = normalize((viewData.camInvertView * vec4(viewDir, 0.0)).xyz);
	vec3 worldPos = convertToAtmosphereUnit(viewData.camWorldPos.xyz) + vec3(0.0, atmosphere.bottomRadius, 0.0);

    float depthBufferValue = -1.0;
    float viewHeight = length(worldPos);

	vec3 L = vec3(0);
	depthBufferValue = texture(sampler2D(inDepth, pointClampEdgeSampler), uv).r;

    float shadingModelId = texture(sampler2D(inGBufferA, pointClampEdgeSampler), uv).a;
    const bool bShadingModelValid = isShadingModelValid(shadingModelId);

    const vec3 sunDirection = -normalize(frameData.directionalLight.direction);

    const bool bUnderAtmosphere =  viewHeight < atmosphere.topRadius;

    vec3 sunLum = drawSun(worldDir, sunDirection);
    {
        // Use smoothstep to limit the effect, so it drops off to actual zero.
        sunLum = smoothstep(0.002, 1.0, sunLum);
        if (length(sunLum) > 0.0) 
        {
            bool bIntersectGround = raySphereIntersectNearest(worldPos, worldDir, vec3(0.0), atmosphere.bottomRadius) >= 0.0f;
		    if (bIntersectGround)
            {
                sunLum *= 0.0;
            }
        }
    }

    // Back ground and under atmosphere pixel, sample sky view lut.
    if (bUnderAtmosphere && (!bShadingModelValid))
	{
		vec3 upVector = normalize(worldPos);
		float viewZenithCosAngle = dot(worldDir, upVector);

        // Assumes non parallel vectors
		vec3 sideVector = normalize(cross(upVector, worldDir));		

        // aligns toward the sun light but perpendicular to up vector
		vec3 forwardVector = normalize(cross(sideVector, upVector));	

		vec2 lightOnPlane = vec2(dot(sunDirection, forwardVector), dot(sunDirection, sideVector));
		lightOnPlane = normalize(lightOnPlane);
		float lightViewCosAngle = lightOnPlane.x;

		bool bIntersectGround = raySphereIntersectNearest(worldPos, worldDir, vec3(0.0), atmosphere.bottomRadius) >= 0.0f;

        vec2 sampleUv;
		skyViewLutParamsToUv(atmosphere, bIntersectGround, viewZenithCosAngle, lightViewCosAngle, viewHeight, sampleUv);

		vec3 luminance = texture(sampler2D(inSkyViewLut, linearClampEdgeSampler), sampleUv).rgb; // TODO: Add sun disk.

		imageStore(imageHdrSceneColor, workPos, vec4(prepareOut(luminance, atmosphere) + sunLum, 1.0f));
        return;
	}

    float opacity = 0.0;
    if(bUnderAtmosphere) // Composite air perspective.
    {
        // Exist pre-compute data, sample it.

        // Build world position.
        clipSpace.z = depthBufferValue;
        vec4 depthBufferWorldPos = viewData.camInvertViewProj * clipSpace;
        depthBufferWorldPos.xyz /= depthBufferWorldPos.w;

        float tDepth = length(convertToAtmosphereUnit(depthBufferWorldPos.xyz) - (worldPos + vec3(0.0, -atmosphere.bottomRadius, 0.0)));
        float slice = aerialPerspectiveDepthToSlice(tDepth);

        float weight = 1.0;
        if (slice < 0.5)
        {
            // We multiply by weight to fade to 0 at depth 0. That works for luminance and opacity.
            weight = saturate(slice * 2.0);
            slice = 0.5;
        }
        ivec3 sliceLutSize = textureSize(inFroxelScatter, 0);
        float w = sqrt(slice / float(sliceLutSize.z));	// squared distribution

        const vec4 airPerspective = weight * texture(sampler3D(inFroxelScatter, linearClampEdgeSampler), vec3(uv, w));
        L.rgb += airPerspective.rgb;
        opacity = airPerspective.a;

        sunLum = vec3(0.0);
    }
    else if(!bShadingModelValid)
    {
        // No precompute data can use. compute new data.

        // Move to top atmosphere as the starting point for ray marching.
        // This is critical to be after the above to not disrupt above atmosphere tests and voxel selection.
        if (!moveToTopAtmosphere(worldPos, worldDir, atmosphere.topRadius))
        {
            // Ray is not intersecting the atmosphere, return.	
            vec3 srcColor = imageLoad(imageHdrSceneColor, workPos).rgb;
            imageStore(imageHdrSceneColor, workPos, vec4(srcColor + sunLum, 1.0f));
            return;
        }

        const bool bGround = false;
        const float sampleCountIni = 0.0;
        const bool bVariableSampleCount = true;
        const bool bMieRayPhase = true;
        const float tMaxMax = kDefaultMaxT;
        depthBufferValue = -1.0;
        SingleScatteringResult ss = integrateScatteredLuminance(
            pixPos, 
            worldPos, 
            worldDir, 
            sunDirection, 
            atmosphere, 
            bGround, 
            sampleCountIni, 
            depthBufferValue, 
            bMieRayPhase,
            tMaxMax,
            bVariableSampleCount
        );

        L += ss.scatteredLight;
        vec3 throughput = ss.transmittance;

        const float transmittance = mean(throughput);
        opacity = 1.0 - transmittance;
    }
    else
    {
        sunLum = vec3(0.0);
    }

    L.rgb = prepareOut(L.rgb, atmosphere);

    vec3 srcColor = imageLoad(imageHdrSceneColor, workPos).rgb;
    vec3 outColor = L.rgb + sunLum + (1.0 - opacity) * srcColor;

    imageStore(imageHdrSceneColor, workPos, vec4(outColor, 1.0f));
}