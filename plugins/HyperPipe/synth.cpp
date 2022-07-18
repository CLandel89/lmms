#include "HyperPipe.h"

#include "NotePlayHandle.h"

namespace lmms
{

HyperPipeNode::HyperPipeNode ()
{
}

HyperPipeNode::~HyperPipeNode ()
{
}

float HyperPipeNoise::processFrame(float freq, float srate)
{
	return 1.0f - fastRandf(2.0f);
}

HyperPipeOsc::HyperPipeOsc () :
		HyperPipeNode()
{
}

HyperPipeOsc::~HyperPipeOsc ()
{
}

float HyperPipeSine::shape(float ph)
{
	float s = sinf(ph * F_2PI);
	float saw = ph; //simplified and reversed
	saw = 1.0f - m_sawify * saw; //ready for multiplication with s
	return saw * s;
}

float HyperPipeOsc::processFrame (float freq, float srate)
{
	m_ph += freq / srate;
	m_ph = fraction(m_ph);
	return shape(m_ph);
}

HyperPipeSynth::HyperPipeSynth (HyperPipe *parent, NotePlayHandle *nph) :
		m_parent(parent),
		m_nph(nph),
		m_lastNode(&myOsc)
{
}

HyperPipeSynth::~HyperPipeSynth ()
{
}

std::array<float,2> HyperPipeSynth::processFrame (float freq, float srate)
{
	m_lastNode->updateFromUI(m_parent);
	float f = m_lastNode->processFrame(freq, srate);
	return {f, f};
}

} // namespace lmms
