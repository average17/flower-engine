#ifndef VIEW_DATA_GLSL
#define VIEW_DATA_GLSL

#ifndef VIEW_DATA_SET
#define VIEW_DATA_SET 0
#endif

#ifndef VIEW_DATA_BINDING
#define VIEW_DATA_BINDING 0
#endif

layout(set = VIEW_DATA_SET, binding = VIEW_DATA_BINDING) uniform ViewBuffer
{
	vec4 camWorldPos;
	vec4 camInfo; // .x fovy, .y aspectRatio, .z nearZ, .w farZ

    mat4 camView; 
	mat4 camProj;
	mat4 camViewProj;

	mat4 camViewInverse;
	mat4 camProjInverse;
	mat4 camViewProjInverse;

	// prev frame camera view project matrix.
	// no jitter.
	mat4 camViewProjLast;

	mat4 camViewProjJitter; 
	mat4 camProjJitter;
	mat4 camProjJitterInverse;
    mat4 camInvertViewProjectionJitter;
	vec4 frustumPlanes[6];
} viewData;

#endif