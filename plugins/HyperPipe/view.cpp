/*
 * view.cpp - implementation of the user interface
 *
 * HyperPipe - synth with arbitrary possibilities
 *
 * Copyright (c) 2022 Christian Landel
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#include "HyperPipe.h"

namespace lmms::gui::hyperpipe
{

HPView::HPView(HyperPipe* instrument, QWidget* parent) :
		InstrumentView(instrument, parent)
{
	auto curNode = instrument->m_model.m_nodes.back();
	if (curNode->name() == "noise") {
		m_curNode = make_unique<HPNoiseView>(
			this,
			instrument,
			static_pointer_cast<HPModel::Noise>(curNode)
		);
	}
	else if (curNode->name() == "shapes") {
		m_curNode = make_unique<HPShapesView>(
			this,
			instrument,
			static_pointer_cast<HPModel::Shapes>(curNode)
		);
	}
	else if (curNode->name() == "sine") {
		m_curNode = make_unique<HPSineView>(
			this,
			instrument,
			static_pointer_cast<HPModel::Sine>(curNode)
		);
	}
	else {
		throw "view implementation missing for HyperPipe " + curNode->name();
	}
}

HPNodeView::~HPNodeView()
{
}

HPNoiseView::HPNoiseView(HPView* view, HyperPipe* instrument, shared_ptr<HPModel::Noise> model) :
		m_spike(view, "spike")
{
	m_spike.setModel(model->m_spike.get());
}
void HPNoiseView::hide() {
	m_spike.hide();
}
void HPNoiseView::show() {
	m_spike.show();
}
string HPNoiseView::name() {
	return "noise";
}

HPSineView::HPSineView(HPView* view, HyperPipe* instrument, shared_ptr<HPModel::Sine> model) :
		m_sawify(view, "sawify")
{
	m_sawify.setModel(model->m_sawify.get());
}
void HPSineView::hide() {
	m_sawify.hide();
}
void HPSineView::show() {
	m_sawify.show();
}
string HPSineView::name() {
	return "sine";
}

HPShapesView::HPShapesView(HPView* view, HyperPipe* instrument, shared_ptr<HPModel::Shapes> model) :
		m_shape(view, "shape"),
		m_jitter(view, "jitter")
{
	m_shape.setModel(model->m_shape.get());
	m_jitter.setModel(model->m_jitter.get());
	m_jitter.move(40, 0);
}
void HPShapesView::hide() {
	m_shape.hide();
	m_jitter.hide();
}
void HPShapesView::show() {
	m_shape.show();
	m_jitter.show();
}
string HPShapesView::name() {
	return "shapes";
}

HPView::~HPView()
{
}

} // namespace lmms::gui::hyperpipe
