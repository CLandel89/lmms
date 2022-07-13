#include "HyperPipe.h"

#include "lmms_math.h"

#include <cmath>

namespace lmms
{

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
	float noise = 1.0f - fastRandf(2.0f);
	noise *= 1.0f - std::abs(s); //how much "room" on top or at the bottom?
	return s + morph * noise;
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
	switch (shape)
	{
		case HyperPipeShapes::NOISE: return 1.0f - fastRandf(2.0f);
		case HyperPipeShapes::SAW:   return saw(ph, morph);
		case HyperPipeShapes::SINE:  return sine(ph, morph);
		case HyperPipeShapes::SQR:   return sqr(ph, morph);
		case HyperPipeShapes::TRI:   return tri(ph, morph);
	}
}

}