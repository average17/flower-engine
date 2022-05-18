#ifndef SHADER_COMMON_GLSL
#define SHADER_COMMON_GLSL

// same with cpp
#define MAX_SSBO_OBJECTS 20000

// we use reverse z, back ground approach to 0.
#define BG_DEPTH 1e-6f

         
// SSBO upload each draw call mesh data
struct PerObjectData   
{
	mat4 modelMatrix;
    mat4 prevModelMatrix;

    // .xyz is localspace center pos
    // .w   sphere radius
    vec4 sphereBounds;  
                       
    // .xyz extent XYZ
    // .w   pad
    vec4 extents;       
                        
    // indirect draw call data
    uint indexCount;    
	uint firstIndex;    
	uint vertexOffset;  
	uint firstInstance; 
};

// Gpu culling result
struct IndexedIndirectCommand 
{
	uint indexCount;
	uint instanceCount;
	uint firstIndex;
	uint vertexOffset;

	uint firstInstance;
    uint objectId;
};

// SSBO upload each draw call material data

// NOTE: it's no a good idea to store all data in one structure.
// TODO: add more sparse shading model structure.

#define SHADING_MODEL_DELTA 0.2f

bool matchShadingModel(float shadingModelVal, float matchingModel)
{
    return abs(shadingModelVal - matchingModel) < SHADING_MODEL_DELTA;
}

#define SHADING_MODEL_UNVALID   0

// shading model id, same with EShadingModel on cpp.
#define SHADING_MODEL_DisneyBRDF 1

// End shading model.

struct PerObjectMaterialData 
{
    uint shadingModel;
    float cutoff;

    uint baseColorId;
    uint baseColorSampler;
    
    uint normalTexId;   
    uint normalSampler;

    uint specularTexId; 
    uint specularSampler;

    uint emissiveTexId; 
    uint emissiveSampler;
};

// end material.

struct IndirectDrawCount
{
    uint count;
};

struct CascadeInfo
{
    mat4  cascadeViewProjMatrix;
    vec4 frustumPlanes[6];
};

struct GbufferData
{
    vec3 baseColor;
    vec3 emissiveColor;

    vec3 worldPos;
    vec3 worldNormal;

    float metal;
    float roughness;
    vec3  ao;
    float so;

    float deviceDepth; // sample device z.

    vec3 F0;
    float shadingModelId;
};

float max4(vec4 inv)
{
    return max(max(max(inv.x, inv.y), inv.z), inv.w);
}

float min4(vec4 inv)
{
    return min(min(min(inv.x, inv.y), inv.z), inv.w);
}

float hash1( uint n ) 
{
    // integer hash copied from Hugo Elias
	n = (n << 13U) ^ n;
    n = n * (n * n * 15731U + 789221U) + 1376312589U;
    return float( n & uint(0x7fffffffU))/float(0x7fffffff);
}

vec3 hash3( uint n ) 
{
    // integer hash copied from Hugo Elias
	n = (n << 13U) ^ n;
    n = n * (n * n * 15731U + 789221U) + 1376312589U;
    uvec3 k = n * uvec3(n,n*16807U,n*48271U);
    return vec3( k & uvec3(0x7fffffffU))/float(0x7fffffff);
}

float linearizeDepth(float z, float n, float f)
{
    return n * f / (z * (f - n) + n);
}

// l = n * f / (z * (f - n) + n)
// (z * (f - n) + n) = n * f / l
// z * (f - n) = n * f / l - n
// z = (n * f / l - n) / (f - n)

float linearZToDeviceZ(float l, float n, float f)
{
    return (n * f / l - n) / (f - n);
}

bool isValid(float x)
{
    return (!isnan(x)) && (!isinf(x));
}

mat4 lookAtRH(vec3 eye,vec3 center,vec3 up)
{
    const vec3 f = normalize(center - eye);
    const vec3 s = normalize(cross(f, up));
    const vec3 u = cross(s, f);

    mat4 ret =  {
        {1.0f,0.0f,0.0f,0.0f},
        {0.0f,1.0f,0.0f,0.0f},
        {0.0f,0.0f,1.0f,0.0f},
        {1.0f,0.0f,0.0f,1.0f}
    };

    ret[0][0] = s.x;
    ret[1][0] = s.y;
    ret[2][0] = s.z;
    ret[0][1] = u.x;
    ret[1][1] = u.y;
    ret[2][1] = u.z;
    ret[0][2] =-f.x;
    ret[1][2] =-f.y;
    ret[2][2] =-f.z;
    ret[3][0] =-dot(s, eye);
    ret[3][1] =-dot(u, eye);
    ret[3][2] = dot(f, eye);

    return ret;
}

mat4 lookAtLH(vec3 eye,vec3 center,vec3 up)
{
    vec3 f = (normalize(center - eye));
    vec3 s = (normalize(cross(up, f)));
    vec3 u = (cross(f, s));

    mat4 Result =  {
        {1.0f,0.0f,0.0f,0.0f},
        {0.0f,1.0f,0.0f,0.0f},
        {0.0f,0.0f,1.0f,0.0f},
        {1.0f,0.0f,0.0f,1.0f}
    };
    Result[0][0] = s.x;
    Result[1][0] = s.y;
    Result[2][0] = s.z;
    Result[0][1] = u.x;
    Result[1][1] = u.y;
    Result[2][1] = u.z;
    Result[0][2] = f.x;
    Result[1][2] = f.y;
    Result[2][2] = f.z;
    Result[3][0] = -dot(s, eye);
    Result[3][1] = -dot(u, eye);
    Result[3][2] = -dot(f, eye);
    return Result;
}

mat4 orthoRH_ZO(float left, float right, float bottom, float top, float zNear, float zFar)
{
    mat4 ret =  {
        {1.0f,0.0f,0.0f,0.0f},
        {0.0f,1.0f,0.0f,0.0f},
        {0.0f,0.0f,1.0f,0.0f},
        {1.0f,0.0f,0.0f,1.0f}
    };

    ret[0][0] = 2.0f / (right - left);
    ret[1][1] = 2.0f / (top - bottom);
    ret[2][2] = -1.0f / (zFar - zNear);
    ret[3][0] = -(right + left) / (right - left);
    ret[3][1] = -(top + bottom) / (top - bottom);
    ret[3][2] = -zNear / (zFar - zNear);

	return ret;
}

mat4 orthoLH_ZO(float left, float right, float bottom, float top, float zNear, float zFar)
{
    mat4 Result =  {
        {1.0f,0.0f,0.0f,0.0f},
        {0.0f,1.0f,0.0f,0.0f},
        {0.0f,0.0f,1.0f,0.0f},
        {1.0f,0.0f,0.0f,1.0f}
    };
    Result[0][0] = 2.0f / (right - left);
    Result[1][1] = 2.0f / (top - bottom);
    Result[2][2] = 1.0f / (zFar - zNear);
    Result[3][0] = - (right + left) / (right - left);
    Result[3][1] = - (top + bottom) / (top - bottom);
    Result[3][2] = - zNear / (zFar - zNear);
    return Result;
}

#define setbit(x,y) (x)|=(1<<(y)) 
#define clrbit(x,y) (x)&=!(1<<(y)) 

bool isSaturated(float x)
{
    return x >= 0.0f && x <= 1.0f;
}

bool isSaturated(vec2 x)
{
    return isSaturated(x.x) && isSaturated(x.y);
}

bool isSaturated(vec3 x)
{
    return isSaturated(x.x) && isSaturated(x.y) && isSaturated(x.z);
}

bool isSaturated(vec4 x)
{
    return isSaturated(x.x) && isSaturated(x.y) && isSaturated(x.z) && isSaturated(x.w);
}

bool onRange(float x, float minV, float maxV)
{
    return x >= minV && x <= maxV;
}

bool onRange(vec2 x, vec2 minV, vec2 maxV)
{
    return onRange(x.x, minV.x, maxV.x) && onRange(x.y, minV.y, maxV.y);
}

bool onRange(vec3 x, vec3 minV, vec3 maxV)
{
    return onRange(x.x, minV.x, maxV.x) && onRange(x.y, minV.y, maxV.y) && onRange(x.z, minV.z, maxV.z);
}

bool onRange(vec4 x, vec4 minV, vec4 maxV)
{
    return onRange(x.x, minV.x, maxV.x) && onRange(x.y, minV.y, maxV.y) && onRange(x.z, minV.z, maxV.z) && onRange(x.w, minV.w, maxV.w);
}

float screenFade(vec2 uv)
{
    vec2 fade = max(vec2(0.0f), 12.0f * abs(uv - 0.5f) - 5.0f);
    return clamp(1.0f - dot(fade, fade), 0.0f, 1.0f);
}

vec2 vogelDisk(uint id, uint count, float angle)
{
  const float goldenAngle = 2.399963f; // radians
  float r = sqrt(id + 0.5f) / sqrt(count);
  float theta = id * goldenAngle + angle;

  float sine = sin(theta);
  float cosine = cos(theta);

  return vec2(cosine, sine) * r;
}

float luma4(vec3 color)
{
    return (color.g * 2) + (color.r + color.b);
}

float hdrWeight4(vec3 color, float exposure)
{
    return 1.0f / (luma4(color) * exposure + 4.0f);
}


// high frequency dither pattern appearing almost random without banding steps
//note: from "NEXT GENERATION POST PROCESSING IN CALL OF DUTY: ADVANCED WARFARE"
//      http://advances.realtimerendering.com/s2014/index.html
// Epic extended by FrameId
// ~7 ALU operations (2 frac, 3 mad, 2 *)
// @return 0..1
// TAA jitter noise iterleave gradient
float interleavedGradientNoiseTAA(vec2 screenPos, uint frameCountModTAA, uint modCount)
{
    float frameStep = float(frameCountModTAA) / float(modCount);

    screenPos.x += frameStep * 4.7526;
    screenPos.y += frameStep * 3.1914;

    vec3 magic = vec3(0.06711056f, 0.00583715f, 52.9829189f);
    return fract(magic.z * fract(dot(screenPos, magic.xy)));
}

// 3D random number generator inspired by PCGs (permuted congruential generator)
// Using a **simple** Feistel cipher in place of the usual xor shift permutation step
// @param v = 3D integer coordinate
// @return three elements w/ 16 random bits each (0-0xffff).
// ~8 ALU operations for result.x    (7 mad, 1 >>)
// ~10 ALU operations for result.xy  (8 mad, 2 >>)
// ~12 ALU operations for result.xyz (9 mad, 3 >>)
uvec3 Rand3DPCG16(ivec3 p)
{
	// taking a signed int then reinterpreting as unsigned gives good behavior for negatives
	uvec3 v = uvec3(p);

	// Linear congruential step. These LCG constants are from Numerical Recipies
	// For additional #'s, PCG would do multiple LCG steps and scramble each on output
	// So v here is the RNG state
	v = v * 1664525u + 1013904223u;

	// PCG uses xorshift for the final shuffle, but it is expensive (and cheap
	// versions of xorshift have visible artifacts). Instead, use simple MAD Feistel steps
	//
	// Feistel ciphers divide the state into separate parts (usually by bits)
	// then apply a series of permutation steps one part at a time. The permutations
	// use a reversible operation (usually ^) to part being updated with the result of
	// a permutation function on the other parts and the key.
	//
	// In this case, I'm using v.x, v.y and v.z as the parts, using + instead of ^ for
	// the combination function, and just multiplying the other two parts (no key) for 
	// the permutation function.
	//
	// That gives a simple mad per round.
	v.x += v.y*v.z;
	v.y += v.z*v.x;
	v.z += v.x*v.y;
	v.x += v.y*v.z;
	v.y += v.z*v.x;
	v.z += v.x*v.y;

	// only top 16 bits are well shuffled
	return v >> 16u;
}

vec2 Hammersley16(uint Index, uint NumSamples, uvec2 Random)
{
	float E1 = fract(float(Index) / NumSamples + float(Random.x) * (1.0 / 65536.0));
	float E2 = float((bitfieldReverse(Index) >> 16 ) ^ Random.y) * (1.0 / 65536.0);
	return vec2(E1, E2);
}

float luminance(vec3 color)
{
    return max(dot(color, vec3(0.299f, 0.587f, 0.114f)), 0.0001f);
}

float radicalInverse(uint i) 
{
    i = (i << 16u) | (i >> 16u);
    i = ((i & 0x55555555u) << 1u) | ((i & 0xAAAAAAAAu) >> 1u);
    i = ((i & 0x33333333u) << 2u) | ((i & 0xCCCCCCCCu) >> 2u);
    i = ((i & 0x0F0F0F0Fu) << 4u) | ((i & 0xF0F0F0F0u) >> 4u);
    i = ((i & 0x00FF00FFu) << 8u) | ((i & 0xFF00FF00u) >> 8u);
    return float(i) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 UniformSampleDisk(vec2 E )
{
	float Theta = 2 * 3.14159265359 * E.x;
	float Radius = sqrt(E.y);
	return Radius * vec2(cos(Theta), sin(Theta));
}

// Brian Karis, Epic Games "Real Shading in Unreal Engine 4"
vec4 ImportanceSampleGGX(vec2 Xi, float Roughness)
{
	float m = Roughness * Roughness;
	float m2 = m * m;

	float Phi = 2 * 3.14159265359 * Xi.x;

	float CosTheta = sqrt((1.0 - Xi.y) / (1.0 + (m2 - 1.0) * Xi.y));
	float SinTheta = sqrt(max(1e-5, 1.0 - CosTheta * CosTheta));

	vec3 H;
	H.x = SinTheta * cos(Phi);
	H.y = SinTheta * sin(Phi);
	H.z = CosTheta;

	float d = (CosTheta * m2 - CosTheta) * CosTheta + 1;
	float D = m2 / (3.14159265359 * d * d);
	float pdf = D * CosTheta;

	return vec4(H, pdf);
}

float GlodenRatioSequence(float seed, uint i) 
{
    float GlodenRatio = 1.618033988749895f;
    return fract(seed + i * GlodenRatio);
}

float interleavedGradientNoise(vec2 screenPos) 
{
    vec3 magic = vec3(0.06711056, 0.00583715, 52.9829189);
    return fract(magic.z * fract(dot(screenPos, magic.xy)));
}

void constructTBN(vec3 n, out vec3 t, out vec3 b) 
{
    vec3 v = abs(n.y) < 0.96 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    t = normalize(cross(n, v));
    b = normalize(cross(n, t));
}

#endif