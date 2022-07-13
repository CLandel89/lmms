#include "HyperPipe.h"
#include "NotePlayHandle.h"

namespace lmms {

HyperPipeNode::HyperPipeNode (Model* _parent) :
        Model(_parent)
{
}

HyperPipeNode::~HyperPipeNode () {
}

HyperPipeOsc::HyperPipeOsc (Model* _model) :
        HyperPipeNode(_model)
{
}

HyperPipeOsc::~HyperPipeOsc () {
}

HyperPipeSynth::HyperPipeSynth (HyperPipe *_i, NotePlayHandle *_nph) :
        m_parent(_i),
        m_nph(_nph)
{
}

HyperPipeSynth::~HyperPipeSynth () {}

std::array<float,2> HyperPipeSynth::processFrame (float freq, float srate)
{
    m_ph += freq / srate;
    m_ph -= floor(m_ph);
    float l = m_ph<=0.7 ? 1.0 : -1.0;
    float r = m_ph<=0.6 ? 1.0 : -1.0;
    return {l, r};
}

} // namespace lmms