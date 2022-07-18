#include "HyperPipe.h"

namespace lmms::gui
{

HyperPipeView::HyperPipeView(HyperPipe* instrument, QWidget* parent) :
		InstrumentView(instrument, parent),
		m_shape(this, "shape"),
		m_jitter(this, "jitter")
{
	m_shape.setModel(&instrument->m_shape);
	m_jitter.setModel(&instrument->m_jitter);
	m_jitter.move(40, 0);
}

HyperPipeView::~HyperPipeView()
{
}

} // namespace gui
