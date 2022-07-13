#include "HyperPipe.h"

#include "NotePlayHandle.h"

namespace lmms
{

HyperPipeNode::HyperPipeNode (Model* parent) :
		Model(parent)
{
}

HyperPipeNode::~HyperPipeNode ()
{
}

HyperPipeOsc::HyperPipeOsc (Model* model) :
		HyperPipeNode(model)
{
}

HyperPipeOsc::~HyperPipeOsc ()
{
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
	m_ph += freq / srate;
	m_ph = fraction(m_ph);
	float l = m_ph<=0.7 ? 1.0 : -1.0;
	float r = m_ph<=0.6 ? 1.0 : -1.0;
	return {l, r};
}

} // namespace lmms
