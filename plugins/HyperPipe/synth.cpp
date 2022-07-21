#include "HyperPipe.h"

namespace lmms
{

HyperPipeNode::HyperPipeNode()
{
}

HyperPipeNode::~HyperPipeNode()
{
}

HyperPipeOsc::HyperPipeOsc() :
		HyperPipeNode()
{
}

HyperPipeOsc::~HyperPipeOsc()
{
}

float HyperPipeOsc::processFrame(float freq, float srate) {
	m_ph += freq / srate;
	m_ph = fraction(m_ph);
	return shape(m_ph);
}

HyperPipeSine::HyperPipeSine(shared_ptr<HyperPipeModel::Sine> model) :
		m_sawify(model->m_sawify)
{
}

HyperPipeSine::~HyperPipeSine()
{
}

float HyperPipeSine::shape(float ph) {
	float s = sinf(ph * F_2PI);
	float saw = ph; //simplified and reversed
	saw = 1.0f - m_sawify->value() * saw; //ready for multiplication with s
	return saw * s;
}

HyperPipeNoise::HyperPipeNoise(shared_ptr<HyperPipeModel::Noise> model, Instrument* instrument) :
		m_spike(model->m_spike),
		m_osc(make_shared<HyperPipeModel::Sine>(instrument))
{
	m_osc.m_sawify->setValue(1.0f);
}

HyperPipeNoise::~HyperPipeNoise()
{
}

float HyperPipeNoise::processFrame(float freq, float srate) {
	float osc = m_osc.processFrame(freq, srate);
	osc = (osc + 1.0f) / 2.0f; //0.0...1.0
	osc = powf(osc, m_spike->value());
	float r = 1.0f - fastRandf(2.0f);
	return osc * r;
}

HyperPipeSynth::HyperPipeSynth(HyperPipe* instrument, NotePlayHandle* nph, HyperPipeModel* model) :
		m_instrument(instrument),
		m_nph(nph),
		m_lastNode(model->m_nodes.back()->instantiate(model->m_nodes.back()))
{
}

HyperPipeSynth::~HyperPipeSynth()
{
}

array<float,2> HyperPipeSynth::processFrame(float freq, float srate) {
	float f = m_lastNode->processFrame(freq, srate);
	return {f, f};
}

} // namespace lmms
