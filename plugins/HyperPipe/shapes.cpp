#include "HyperPipe.h"

namespace lmms
{

inline float saw2tri(float ph, float morph)
{
	// left and right edge of each segment
	float le, re;
	le = 0.0f;
	re = morph * 0.25f;
	if (ph < re) {
		//0.0...1.0
		return ph / re;
	}
	le = re;
	re = 1.0f - morph * 0.25f;
	if (ph < re) {
		//this is the main (saw) shape
		//1.0...-1.0
		return 1.0f - 2.0f * (ph - le) / (re - le);
	}
	le = re;
	re = 1.0f;
	//-1.0...0.0
	return -1.0f + (ph - le) / (re - le);
}

inline float sqr2saw(float ph, float morph)
{
	// left and right edge of each segment
	float le, re;
	le = 0.0f;
	re = 0.5f - morph * 0.5f;
	if (ph < re) {
		return 1.0f;
	}
	le = re;
	re = 0.5f + morph * 0.5f;
	if (ph < re) {
		return 1.0f - 2.0f * (ph - le) / (re - le);
	}
	//re = 1.0f;
	return -1.0f;
}

inline float tri2sqr(float ph, float morph)
{
	// left and right edge of each segment
	float le, re;
	le = 0.0;
	re = 0.25f - morph * 0.25f;
	if (ph < re) {
		return ph / re;
	}
	//le = re;
	re = 0.25f + morph * 0.25f;
	if (ph < re) {
		return 1.0f;
	}
	le = re;
	re = 0.75f - morph * 0.25f;
	if (ph < re) {
		return 1.0f - 2.0f * (ph - le) / (re - le);
	}
	le = re;
	re = 0.75f + morph * 0.25f;
	if (ph < re) {
		return -1.0f;
	}
	le = re;
	re = 1.0f;
	return -1.0f + (ph - le) / (re - le);
}

HyperPipeShapes::HyperPipeShapes()
{
}

HyperPipeShapes::~HyperPipeShapes()
{
}

float HyperPipeShapes::shape(float ph)
{
	float shape = m_shape + fastRandf(m_jitter);
	while (shape < 0.0f) { shape += 3.0f; }
	while (shape >= 3.0f) { shape -= 3.0f; }
	float morph = fraction(shape);
	// amp: the shapes with vertical edges sound too loud
	if (shape < 1.0f) {
		float amp = 0.4f + 0.6f * morph;
		return amp * saw2tri(ph, morph);
	}
	if (shape < 2.0f) {
		float amp = 1.0f - 0.7f * morph;
		return amp * tri2sqr(ph, morph);
	}
	float amp = 0.3f + 0.1f * morph;
	return amp * sqr2saw(ph, morph);
}

void HyperPipeShapes::updateFromUI(HyperPipe* instrument)
{
	m_shape = instrument->m_shape.value();
	m_jitter = instrument->m_jitter.value();
}

}