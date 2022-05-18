#ifndef VOLUMETRIC_GLSL
#define VOLUMETRIC_GLSL

// see https://en.wikipedia.org/wiki/Beer%E2%80%93Lambert_law
// return transmittance.
float beersLaw(float density, float factor)
{
    return exp(-density * factor);
}

// HG Effect.
float henyeyGreenstein( float sundotrd, float g) 
{
	float gg = g * g;
	return (1. - gg) / pow( 1. + gg - 2. * g * sundotrd, 1.5);
}

#endif