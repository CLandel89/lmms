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

HyperPipeSynth::~HyperPipeSynth () {
}

} // namespace lmms