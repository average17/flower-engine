#ifndef SAMPLER_GLSL
#define SAMPLER_GLSL

vec4 upscaleSampleWith4TapBlur(sampler2D inTex, vec2 uv, vec2 texelSize)
{
    // color offsets.
	vec4 c00 = texture(inTex, uv + vec2(0.0, 0.0) * texelSize);
	vec4 c01 = texture(inTex, uv + vec2(0.0, 1.0) * texelSize);
	vec4 c10 = texture(inTex, uv + vec2(1.0, 0.0) * texelSize);
	vec4 c11 = texture(inTex, uv + vec2(1.0, 1.0) * texelSize);

    return 0.25f * (c00 + c01 + c10 + c11);
}

// catmull Rom 9 tap sampler.
// sTex: linear clamp sampler2D.
// uv: sample uv.
// resolution: working rt resolution.
vec3 catmullRom9Sample(sampler2D sTex, vec2 uv, vec2 resolution)
{
    vec2 samplePos = uv * resolution;
    vec2 texPos1   = floor(samplePos - 0.5f) + 0.5f;

    vec2 f = samplePos - texPos1;

    vec2 w0 = f * (-0.5f + f * (1.0f - 0.5f * f));
    vec2 w1 = 1.0f + f * f * (-2.5f + 1.5f * f);
    vec2 w2 = f * (0.5f + f * (2.0f - 1.5f * f));
    vec2 w3 = f * f * (-0.5f + 0.5f * f);

    vec2 w12 = w1 + w2;
    vec2 offset12 = w2 / (w1 + w2);

    vec2 texPos0 = texPos1 - 1.0f;
    vec2 texPos3 = texPos1 + 2.0f;

    vec2 texPos12 = texPos1 + offset12;

    texPos0  /= resolution;
    texPos3  /= resolution;
    texPos12 /= resolution;

    vec3 result = vec3(0.0f);

    result += textureLod(sTex, vec2(texPos0.x,  texPos0.y),  0).xyz * w0.x  * w0.y;
    result += textureLod(sTex, vec2(texPos12.x, texPos0.y),  0).xyz * w12.x * w0.y;
    result += textureLod(sTex, vec2(texPos3.x,  texPos0.y),  0).xyz * w3.x  * w0.y;

    result += textureLod(sTex, vec2(texPos0.x,  texPos12.y), 0).xyz * w0.x  * w12.y;
    result += textureLod(sTex, vec2(texPos12.x, texPos12.y), 0).xyz * w12.x * w12.y;
    result += textureLod(sTex, vec2(texPos3.x,  texPos12.y), 0).xyz * w3.x  * w12.y;

    result += textureLod(sTex, vec2(texPos0.x,  texPos3.y),  0).xyz * w0.x  * w3.y;
    result += textureLod(sTex, vec2(texPos12.x, texPos3.y),  0).xyz * w12.x * w3.y;
    result += textureLod(sTex, vec2(texPos3.x,  texPos3.y),  0).xyz * w3.x  * w3.y;

    return max(result, vec3(0.0f));
}

// depth guide upscale sampler.
vec4 depthAwareUpsampler(vec2 uv, vec2 texelSize,  sampler2D depthTex, sampler2D inTex, float depthAwareFactor, float zMin, float zMax)
{
    // color offsets.
	vec4 c00 = texture(inTex, uv + vec2(0.0, 0.0) * texelSize);
	vec4 c01 = texture(inTex, uv + vec2(0.0, 1.0) * texelSize);
	vec4 c10 = texture(inTex, uv + vec2(1.0, 0.0) * texelSize);
	vec4 c11 = texture(inTex, uv + vec2(1.0, 1.0) * texelSize);

	// depth offset.
	float z00 = linearizeDepth(texture(depthTex, uv + vec2(0.0, 0.0) * texelSize).r, zMin, zMax);
	float z01 = linearizeDepth(texture(depthTex, uv + vec2(0.0, 1.0) * texelSize).r, zMin, zMax);
	float z10 = linearizeDepth(texture(depthTex, uv + vec2(1.0, 0.0) * texelSize).r, zMin, zMax);
	float z11 = linearizeDepth(texture(depthTex, uv + vec2(1.0, 1.0) * texelSize).r, zMin, zMax);

	vec4 zz = vec4(z00, z01, z10, z11);
	vec4 weights = clamp(1.0 - abs(zz - z00) / depthAwareFactor, vec4(0), vec4(1.0));

	vec2 uvMixFactor = fract(uv / texelSize);
	vec4 c00c01 = mix(c00 * weights.x, c01 * weights.y, uvMixFactor.x);
	vec4 c11c10 = mix(c11 * weights.w, c10 * weights.z, uvMixFactor.x);
	return mix(c00c01, c11c10, uvMixFactor.y);
}

vec4 depthAwareUpsampler4(vec2 uv, vec2 texelSize,  sampler2D depthTex, sampler2D inTex, vec4 depthAwareFactor, float zMin, float zMax)
{
    // color offsets.
	vec4 c00 = texture(inTex, uv + vec2(0.0, 0.0) * texelSize);
	vec4 c01 = texture(inTex, uv + vec2(0.0, 1.0) * texelSize);
	vec4 c10 = texture(inTex, uv + vec2(1.0, 0.0) * texelSize);
	vec4 c11 = texture(inTex, uv + vec2(1.0, 1.0) * texelSize);

	// depth offset.
	float z00 = linearizeDepth(texture(depthTex, uv + vec2(0.0, 0.0) * texelSize).r, zMin, zMax);
	float z01 = linearizeDepth(texture(depthTex, uv + vec2(0.0, 1.0) * texelSize).r, zMin, zMax);
	float z10 = linearizeDepth(texture(depthTex, uv + vec2(1.0, 0.0) * texelSize).r, zMin, zMax);
	float z11 = linearizeDepth(texture(depthTex, uv + vec2(1.0, 1.0) * texelSize).r, zMin, zMax);

    float x1;
    float y1;
    float z1;
    float w1;
    vec4 zz = vec4(z00, z01, z10, z11);
    vec2 uvMixFactor = fract(uv / texelSize);
    {
        vec4 weights = clamp(1.0 - abs(zz - z00) / depthAwareFactor.x, vec4(0), vec4(1.0));
        vec4 c00c01 = mix(c00 * weights.x, c01 * weights.y, uvMixFactor.x);
        vec4 c11c10 = mix(c11 * weights.w, c10 * weights.z, uvMixFactor.x);
        x1 = mix(c00c01, c11c10, uvMixFactor.y).x;
    }
    {
        
        vec4 weights = clamp(1.0 - abs(zz - z00) / depthAwareFactor.y, vec4(0), vec4(1.0));
        vec4 c00c01 = mix(c00 * weights.x, c01 * weights.y, uvMixFactor.x);
        vec4 c11c10 = mix(c11 * weights.w, c10 * weights.z, uvMixFactor.x);
        y1 = mix(c00c01, c11c10, uvMixFactor.y).y;
    }
    {
        vec4 weights = clamp(1.0 - abs(zz - z00) / depthAwareFactor.z, vec4(0), vec4(1.0));
        vec4 c00c01 = mix(c00 * weights.x, c01 * weights.y, uvMixFactor.x);
        vec4 c11c10 = mix(c11 * weights.w, c10 * weights.z, uvMixFactor.x);
        z1 = mix(c00c01, c11c10, uvMixFactor.y).z;
    }
    {
        vec4 weights = clamp(1.0 - abs(zz - z00) / depthAwareFactor.w, vec4(0), vec4(1.0));
        vec4 c00c01 = mix(c00 * weights.x, c01 * weights.y, uvMixFactor.x);
        vec4 c11c10 = mix(c11 * weights.w, c10 * weights.z, uvMixFactor.x);
        w1 = mix(c00c01, c11c10, uvMixFactor.y).w;
    }
	
	return vec4(x1,y1,z1,w1);
}


#endif