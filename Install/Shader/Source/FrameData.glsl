#ifndef FRAME_DATA_GLSL
#define FRAME_DATA_GLSL

#ifndef FRAME_DATA_SET
#define FRAME_DATA_SET 0
#endif

#ifndef FRAME_DATA_BINDING
#define FRAME_DATA_BINDING 0
#endif

layout(set = FRAME_DATA_SET, binding = FRAME_DATA_BINDING) uniform FrameBuffer
{
    vec4 appTime;
        // .x is app runtime
        // .y is game runtime
        // .z is sin(game runtime)
        // .w is cos(game runtime)

    vec4 directionalLightDir;
    vec4 directionalLightColor;

    uvec4 frameIndex; // .x is frame count, .y is frame count % 8, .z is frame count % 16.  .w is frame count % 32
    vec4  jitterData; // .xy is current frame jitter data, .zw is prev frame jitter data.

    vec4 switch0; // .x is TAA open
} frameData;

bool TAAOpen()
{
    return frameData.switch0.x > 0.5f;
}

#endif