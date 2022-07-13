#include "HyperPipe.h"

namespace lmms::gui {

HyperPipeView::HyperPipeView(Instrument *_instrument, QWidget *_parent) :
        InstrumentView(_instrument, _parent)
{
}

HyperPipeView::~HyperPipeView() {}

void HyperPipeView::modelChanged() {}

} // namespace gui