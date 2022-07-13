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

HyperPipeOsc::HyperPipeOsc () :
		HyperPipeNode()
{
}

HyperPipeOsc::~HyperPipeOsc ()
{
}

float HyperPipeOsc::processFrame (float freq, float srate)
{
	m_ph += freq / srate;
	m_ph = fraction(m_ph);
	return hyperPipeShape(m_shape, m_ph, m_morph);
}

HyperPipeSynth::HyperPipeSynth (HyperPipe *parent, NotePlayHandle *nph) :
		m_parent(parent),
		m_nph(nph)
{
}

HyperPipeSynth::~HyperPipeSynth ()
{
}

std::array<float,2> HyperPipeSynth::processFrame (float freq, float srate)
{
	m_osc.m_shape = (HyperPipeShapes) m_parent->m_shape.value();
	m_osc.m_morph = m_parent->m_morph.value();
	float f = m_osc.processFrame(freq, srate);
	return {f, f};
}

} // namespace lmms
