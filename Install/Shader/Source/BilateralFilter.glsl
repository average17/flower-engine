#ifndef BILATERAL_FILTER_GLSL
#define BILATERAL_FILTER_GLSL

// TOO Slow For Me.

// M Size config.
#ifndef BILATERAL_FILTER_MSIZE
#define BILATERAL_FILTER_MSIZE 15
#endif

struct BilateralConfig
{
    float sigma;
    float bsigma;
};

BilateralConfig getDefaultBilateralConfig()
{
    BilateralConfig ret;
    ret.sigma = 10.0f;
    ret.bsigma = 0.1f;

    return ret;
}

float normpdf(in float x, in float sigma)
{
	return 0.39894 * exp(-0.5 * x * x / (sigma * sigma)) / sigma;
}

float normpdf3(in vec3 v, in float sigma)
{
	return 0.39894 * exp(-0.5 * dot(v,v) / (sigma * sigma)) / sigma;
}

vec3 bilateralFilter(in BilateralConfig inConfig, in vec2 uv, in vec2 texelSize, in sampler2D inTex)
{
    vec3 finalColor = vec3(0.0);

    const int kSize = (BILATERAL_FILTER_MSIZE - 1) / 2;
	float kernel[BILATERAL_FILTER_MSIZE];

    //create the 1-D kernel
    float Z = 0.0;
    for (int j = 0; j <= kSize; ++j)
    {
        kernel[kSize + j] = kernel[kSize - j] = normpdf(float(j), inConfig.sigma);
    }

    vec3 c = texture(inTex, uv).rgb;

    vec3 cc;
    float factor;
    float bZ = 1.0 / normpdf(0.0, inConfig.bsigma);

    // read out the texels
    for (int i = -kSize; i <= kSize; ++i)
    {
        for (int j = -kSize; j <= kSize; ++j)
        {
            cc = texture(inTex, uv + vec2(i,j) * texelSize).rgb;

            factor = normpdf3(cc - c, inConfig.bsigma) * bZ * kernel[kSize+j] * kernel[kSize+i];
            Z += factor;
            finalColor += factor * cc;
        }
    }

    finalColor = finalColor / Z;

    return finalColor;
}
#endif