#include "HyperPipe.h"

namespace lmms::gui
{

HyperPipeView::HyperPipeView(HyperPipe* instrument, QWidget* parent) :
		InstrumentView(instrument, parent)
{
	auto curNode = instrument->m_model->m_nodes.back();
	if (curNode->name() == "noise") {
		m_curNode = make_unique<HyperPipeNoiseView>(
			this,
			instrument,
			static_pointer_cast<HyperPipeModel::Noise>(curNode)
		);
	}
	else if (curNode->name() == "shapes") {
		m_curNode = make_unique<HyperPipeShapesView>(
			this,
			instrument,
			static_pointer_cast<HyperPipeModel::Shapes>(curNode)
		);
	}
	else if (curNode->name() == "sine") {
		m_curNode = make_unique<HyperPipeSineView>(
			this,
			instrument,
			static_pointer_cast<HyperPipeModel::Sine>(curNode)
		);
	}
	else {
		throw "view implementation missing for HyperPipe " + curNode->name();
	}
}

HyperPipeNodeView::~HyperPipeNodeView()
{
}

HyperPipeNoiseView::HyperPipeNoiseView(HyperPipeView* view, HyperPipe* instrument, shared_ptr<HyperPipeModel::Noise> model) :
		m_spike(view, "spike")
{
	m_spike.setModel(model->m_spike.get());
}
void HyperPipeNoiseView::hide() {
	m_spike.hide();
}
void HyperPipeNoiseView::show() {
	m_spike.show();
}
string HyperPipeNoiseView::name() {
	return "noise";
}

HyperPipeSineView::HyperPipeSineView(HyperPipeView* view, HyperPipe* instrument, shared_ptr<HyperPipeModel::Sine> model) :
		m_sawify(view, "sawify")
{
	m_sawify.setModel(model->m_sawify.get());
}
void HyperPipeSineView::hide() {
	m_sawify.hide();
}
void HyperPipeSineView::show() {
	m_sawify.show();
}
string HyperPipeSineView::name() {
	return "sine";
}

HyperPipeShapesView::HyperPipeShapesView(HyperPipeView* view, HyperPipe* instrument, shared_ptr<HyperPipeModel::Shapes> model) :
		m_shape(view, "shape"),
		m_jitter(view, "jitter")
{
	m_shape.setModel(model->m_shape.get());
	m_jitter.setModel(model->m_jitter.get());
	m_jitter.move(40, 0);
}
void HyperPipeShapesView::hide() {
	m_shape.hide();
	m_jitter.hide();
}
void HyperPipeShapesView::show() {
	m_shape.show();
	m_jitter.show();
}
string HyperPipeShapesView::name() {
	return "shapes";
}

HyperPipeView::~HyperPipeView()
{
}

} // namespace gui
