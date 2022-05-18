#ifndef BINDLESS_GLSL
#define BINDLESS_GLSL

#ifndef BINDLESS_IMAGE_SET
#define BINDLESS_IMAGE_SET 0
#endif

#ifndef BINDLESS_SAMPLER_SET
#define BINDLESS_SAMPLER_SET 1
#endif

layout(set = BINDLESS_IMAGE_SET, binding = 0) uniform texture2D BindlessImage2D[];
layout(set = BINDLESS_SAMPLER_SET, binding = 0) uniform sampler BindlessSampler[];

vec4 texlod(uint texId,uint samplerId,vec2 uv, float lod)
{
    return textureLod(
        sampler2D(BindlessImage2D[nonuniformEXT(texId)], BindlessSampler[nonuniformEXT(samplerId)])
        , uv
        , lod
    );
}

vec4 tex(uint texId,uint samplerId,vec2 uv)
{
    return texture(
        sampler2D(BindlessImage2D[nonuniformEXT(texId)], BindlessSampler[nonuniformEXT(samplerId)])
        , uv
    );
}

#endif