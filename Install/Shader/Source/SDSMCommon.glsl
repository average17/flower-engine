#ifndef SDSM_COMMON_GLSL
#define SDSM_COMMON_GLSL

struct DepthRange
{
    uint minDepth;
    uint maxDepth;
};

// Max  0xFFFFFFFF        4294967295
#define DEPTH_SCALE_UINT  4294967200
#define DEPTH_SCALE_FLOAT 4294967200.0f

// we use uint to pack cascade active flag,
// so use 4 or 8 or 16 or 32 is suitable.
#define MAX_CASCADE_COUNT 8
#define MAX_CASCADE_SSBO_OBJECTS MAX_SSBO_OBJECTS * MAX_CASCADE_COUNT

#endif