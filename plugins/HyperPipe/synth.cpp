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

HyperPipeSine::HyperPipeSine()
{
}

HyperPipeSine::~HyperPipeSine()
{
}

float HyperPipeSine::shape(float ph) {
	float s = sinf(ph * F_2PI);
	float saw = ph; //simplified and reversed
	saw = 1.0f - m_sawify * saw; //ready for multiplication with s
	return saw * s;
}

void HyperPipeSine::updateFromUI(HyperPipe* instrument) {
	// WIP
}

HyperPipeNoise::HyperPipeNoise() {
	m_osc.m_sawify = 1.0f;
}

HyperPipeNoise::~HyperPipeNoise()
{
}

float HyperPipeNoise::processFrame(float freq, float srate) {
	float osc = m_osc.processFrame(freq, srate);
	osc = (osc + 1.0f) / 2.0f; //0.0...1.0
	osc = powf(osc, m_spike);
	float r = 1.0f - fastRandf(2.0f);
	return osc * r;
}

void HyperPipeNoise::updateFromUI(HyperPipe* instrument) {
	// WIP
}

HyperPipeSynth::HyperPipeSynth(HyperPipe* parent, NotePlayHandle* nph) :
		m_parent(parent),
		m_nph(nph),
		m_lastNode(&myOsc)
{
}

HyperPipeSynth::~HyperPipeSynth()
{
}

std::array<float,2> HyperPipeSynth::processFrame(float freq, float srate) {
	m_lastNode->updateFromUI(m_parent);
	float f = m_lastNode->processFrame(freq, srate);
	return {f, f};
}

} // namespace lmms
