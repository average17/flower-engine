#ifndef COMMON_VERTEX_GLSL
#define COMMON_VERTEX_GLSL

#ifdef STANDARD_VERTEX_INPUT
    layout (location = 0) in vec3 inPosition;
    layout (location = 1) in vec2 inUV0;
    layout (location = 2) in vec3 inNormal;
    layout (location = 3) in vec4 inTangent;
#endif

#endif