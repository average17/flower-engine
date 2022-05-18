#ifndef RAY_COMMON_GLSL
#define RAY_COMMON_GLSL

// common ray struct for trace.
struct Ray
{
    vec3 origin;
    vec3 direction;
};

// 3d sphere.
struct Sphere
{
    vec3  center;
    float radius;
};

// ray sphere intersect compute.
// return: intersection t0 and t1,
//         t0 = -t;
//         t1 =  t;
// if one intersect point,  t0 = t1 =  0;
// if no  intersect point,  t0 = t1 = -1;
// if two intersect point, -t0 = t1 =  t;
vec2 raySphereIntersect(Ray ray, Sphere sphere)
{
	vec3 s2r = ray.origin - sphere.center;

	float a = dot(ray.direction, ray.direction);
	float b = 2.0 * dot(ray.direction, s2r);
	float c = dot(s2r, s2r) - (sphere.radius * sphere.radius);
	float delta = b * b - 4.0 * a * c;

	if (delta >= 0.0)
	{
		return (-b + vec2(-1, 1) * sqrt(delta)) / (2.0 * a);
	}
    
	return vec2(-1);
}

#endif