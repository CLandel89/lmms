#include "HyperPipe.h"

#include "lmms_math.h"

#include <cmath>

namespace lmms
{

inline float sine (float ph, float morph);

inline float noise (float ph, float morph)
{
	float r = (1.0f - morph) * fastRandf(1.0f) - 0.5f;
	return sine(ph + r, morph);
}

inline float saw (float ph, float morph)
{
	// left and right edge of each segment
	float le, re;
	le = 0.0f;
	re = morph * 0.25f;
	if (ph < re)
	{
		//0.0...1.0
		return ph / re;
	}
	le = re;
	re = 1.0f - morph * 0.25f;
	if (ph < re)
	{
		//this is the main (saw) shape
		//1.0...-1.0
		return 1.0f - 2.0f * (ph - le) / (re - le);
	}
	le = re;
	re = 1.0f;
	//-1.0...0.0
	return -1.0f + (ph - le) / (re - le);
}

inline float sine (float ph, float morph)
{
	float s = sinf(ph * F_2PI);
	float saw = ph; //a simplified version
	saw = 1.0f - morph * saw;
	return saw * s;
}

inline float sqr (float ph, float morph)
{
	// left and right edge of each segment
	float le, re;
	le = 0.0f;
	re = 0.5f - morph * 0.5f;
	if (ph < re)
	{
		return 1.0f;
	}
	le = re;
	re = 0.5f + morph * 0.5f;
	if (ph < re)
	{
		return 1.0f - 2.0f * (ph - le) / (re - le);
	}
	//re = 1.0f;
	return -1.0f;
}

inline float tri (float ph, float morph)
{
	// left and right edge of each segment
	float le, re;
	le = 0.0;
	re = 0.25f - morph * 0.25f;
	if (ph < re)
	{
		return ph / re;
	}
	//le = re;
	re = 0.25f + morph * 0.25f;
	if (ph < re)
	{
		return 1.0f;
	}
	le = re;
	re = 0.75f - morph * 0.25f;
	if (ph < re)
	{
		return 1.0f - 2.0f * (ph - le) / (re - le);
	}
	le = re;
	re = 0.75f + morph * 0.25f;
	if (ph < re)
	{
		return -1.0f;
	}
	le = re;
	re = 1.0f;
	return -1.0f + (ph - le) / (re - le);
}

float hyperPipeShape (HyperPipeShapes shape, float ph, float morph)
{
	if (morph < 0.5f) {
		morph += fastRandf(0.05f);
	}
	else {
		morph -= fastRandf(0.05f);
	}
	switch (shape)
	{
		//the shapes with vertical edges really sound too loud
		case HyperPipeShapes::NOISE: return noise(ph, morph);
		case HyperPipeShapes::SAW:   return (0.4f + 0.5f * morph) * saw(ph, morph);
		case HyperPipeShapes::SINE:  return sine(ph, morph);
		case HyperPipeShapes::SQR:   return (0.4f - 0.1f * morph) * sqr(ph, morph);
		case HyperPipeShapes::TRI:   return (1.0f - 0.6f * morph) * tri(ph, morph);
	}
}

}