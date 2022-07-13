#include "HyperPipe.h"

namespace lmms::gui
{

HyperPipeView::HyperPipeView(HyperPipe *instrument, QWidget *parent) :
		InstrumentView(instrument, parent),
		m_shape(this, "shape"),
		m_morph(this, "morph")
{
	m_shape.setModel(&instrument->m_shape);
	m_morph.move(100, 0);
	m_morph.setModel(&instrument->m_morph);
}

HyperPipeView::~HyperPipeView()
{
}

void HyperPipeView::modelChanged()
{
}

} // namespace gui
