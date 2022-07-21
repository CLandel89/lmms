#include "HyperPipe.h"

namespace lmms
{

HyperPipeModel::HyperPipeModel(Instrument* instrument)
{
	m_nodes.emplace_back(make_shared<HyperPipeModel::Shapes>(instrument));
}

HyperPipeModel::Noise::Noise(Instrument* instrument) :
		m_spike(make_shared<FloatModel>(12.0f, 0.0f, 20.0f, 0.1f, instrument, tr("spike")))
{
	m_fmodels.emplace_back(m_spike);
}

shared_ptr<HyperPipeNode> HyperPipeModel::Noise::instantiate(shared_ptr<HyperPipeModel::Node> self, Instrument* instrument) {
	return make_shared<HyperPipeNoise>(
		static_pointer_cast<HyperPipeModel::Noise>(self),
		instrument
	);
}

string HyperPipeModel::Noise::name() {
	return "noise";
}

HyperPipeModel::Sine::Sine(Instrument* instrument) :
		m_sawify(make_shared<FloatModel>(0.0f, 0.0f, 1.0f, 0.01f, instrument, tr("sawify")))
{
	m_fmodels.emplace_back(m_sawify);
}

shared_ptr<HyperPipeNode> HyperPipeModel::Sine::instantiate(shared_ptr<HyperPipeModel::Node> self) {
	return make_shared<HyperPipeSine>(
		static_pointer_cast<HyperPipeModel::Sine>(self)
	);
}

string HyperPipeModel::Sine::name() {
	return "sine";
}

HyperPipeModel::Shapes::Shapes(Instrument* instrument) :
		m_shape(make_shared<FloatModel>(0.0f, -3.0f, 3.0f, 0.01f, instrument, tr("shape"))),
		m_jitter(make_shared<FloatModel>(0.0f, -3.0f, 3.0f, 0.01f, instrument, tr("jitter")))
{
	m_fmodels.emplace_back(m_shape);
	m_fmodels.emplace_back(m_jitter);
}

shared_ptr<HyperPipeNode> HyperPipeModel::Shapes::instantiate(shared_ptr<HyperPipeModel::Node> self) {
	return make_shared<HyperPipeShapes>(
		static_pointer_cast<HyperPipeModel::Shapes>(self)
	);
}

string HyperPipeModel::Shapes::name() {
	return "shapes";
}

void HyperPipe::saveSettings (QDomDocument& doc, QDomElement& parent)
{
}

void HyperPipe::loadSettings (const QDomElement& preset)
{
}

} // namespace lmms