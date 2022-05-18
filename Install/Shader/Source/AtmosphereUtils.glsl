#ifndef ATMOSPHERE_UTILS
#define ATMOSPHERE_UTILS


// do white balance when compute sky back ground, we can get a colorful sky when sun set.
// don't when compute lighting and perspective. which make light color bias too much to current image.
const vec3 kWhitePoint = vec3(1.08241, 0.96756, 0.95003);

/*
 * NOTE: Epic sebh atmosphere model base on base on Bruneton08.
 *       Bruneton08 compute sky lighting on gamma space.

 ***************************************************************************************
         Our engine working on linear space, so, remember to do a gamma reverse.
         It is very importance!

         When lighting, compute transmittance, remember to do a reverse. or the result will very yellow.
 ****************************************************************************************
*/

/*
 * Same parameterization here.
 */
vec3 getValFromTLUT(sampler2D tex, vec3 pos, vec3 sunDir, float groundRadiusMM, float atmosphereRadiusMM) 
{
    float height = length(pos);
    vec3 up = pos / height;
	float sunCosZenithAngle = dot(sunDir, up);

    vec2 uv = vec2(
        clamp(0.5 + 0.5 * sunCosZenithAngle, 0.0, 1.0), 
        max(0.0, min(1.0, (height - groundRadiusMM) / (atmosphereRadiusMM - groundRadiusMM)))
    );
    return texture(tex, uv).rgb;
}

// get View pos fit to planet unit.
vec3 getViewPos(float groundRadiusMM, float offsetHeight, vec3 camPos)
{
    const vec3 earthCenter = vec3(0.0);

    // 200M above the ground.
    const vec3 earthBaseLevel = earthCenter + vec3(0.0f, groundRadiusMM + offsetHeight, 0.0f);

    return camPos * 0.001f * 0.001f + earthBaseLevel;
}

// see sebh's github project.
void lutTransmittanceParamsToUv(in float viewHeight, in float viewZenithCosAngle, out vec2 uv, in float topRadius, in float bottomRadius)
{
	float H = sqrt(max(0.0f, topRadius * topRadius - bottomRadius * bottomRadius));
	float rho = sqrt(max(0.0f, viewHeight * viewHeight - bottomRadius * bottomRadius));

	float discriminant = viewHeight * viewHeight * (viewZenithCosAngle * viewZenithCosAngle - 1.0) + topRadius * topRadius;
	float d = max(0.0, (-viewHeight * viewZenithCosAngle + sqrt(discriminant))); // Distance to atmosphere boundary

	float d_min = topRadius - viewHeight;
	float d_max = rho + H;
	float x_mu = (d - d_min) / (d_max - d_min);
	float x_r = rho / H;

	uv = vec2(x_mu, x_r);
	//uv = vec2(fromUnitToSubUvs(uv.x, TRANSMITTANCE_TEXTURE_WIDTH), fromUnitToSubUvs(uv.y, TRANSMITTANCE_TEXTURE_HEIGHT)); // No real impact so off
}

#endif