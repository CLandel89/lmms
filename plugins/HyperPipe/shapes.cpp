#include "HyperPipe.h"

#include "lmms_math.h"

namespace lmms
{

inline float sine (float ph, float morph)
{
	float s = sinf(ph * F_2PI);
	float noise = fastRandf(2.0f) - 1.0f;
	noise *= 1.0f - s * s; //how much "room" on top or at the bottom?
	return s + morph * noise;
}

inline float sqr (float ph, size_t n, float morph)
{}

std::array<float,2> hyperPipeShape (HyperPipeShapes shape, float ph, size_t n, float morph)
{}

}
