#include "HyperPipe.h"

namespace lmms::gui
{

HyperPipeView::HyperPipeView(HyperPipe* instrument, QWidget* parent) :
		InstrumentView(instrument, parent),
		m_shapes(this)
{
	m_shapes.shape.setModel(&instrument->m_shapes.shape);
	m_shapes.jitter.setModel(&instrument->m_shapes.jitter);
	m_shapes.jitter.move(40, 0);
}

HyperPipeView::Shapes::Shapes(HyperPipeView* view) :
		shape(view, "shape"),
		jitter(view, "jitter")
{
}

HyperPipeView::~HyperPipeView()
{
}

} // namespace gui
