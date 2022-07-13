#include "HyperPipe.h"

#include "lmms_math.h"

#include <cmath>

namespace lmms
{

inline float sine (float ph, float morph)
{
	float s = sinf(ph * F_2PI);
	float noise = 1.0f - fastRandf(2.0f);
	noise *= 1.0f - std::abs(s); //how much "room" on top or at the bottom?
	return s + morph * noise;
}

inline float sqr (float ph, size_t n, float morph)
{
	if (morph > 0.999f) { morph = 0.999f; }
	bool pulse;
	if (n % 10 < 5)
	{
		pulse = ph < 0.5f + morph * 0.5f;
	}
	else
	{
		// FM does not like shapes with asymmetric distribution
		pulse = ph >= 0.5f + morph * 0.5f;
	}
	return pulse ? 1.0f : -1.0f;
}

inline float tri (float ph, float morph)
{}

float hyperPipeShape (HyperPipeShapes shape, float ph, size_t n, float morph)
{
	switch (shape)
	{
		case HyperPipeShapes::NOISE: return 1.0f - fastRandf(2.0f);
	}
}

}