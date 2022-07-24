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
		InstrumentView(instrument, parent),
		m_noise(this, instrument),
		m_shapes(this, instrument),
		m_sine(this, instrument),
		m_instrument(instrument),
		m_nodeType(this, "node type")
{
	m_noise.moveRel(0, 25);
	m_noise.hide();
	m_shapes.moveRel(0, 25);
	m_shapes.hide();
	m_sine.moveRel(0, 25);
	m_sine.hide();
	m_nodeTypeModel.addItem(tr("noise"));
	m_nodeTypeModel.addItem(tr("shapes"));
	m_nodeTypeModel.addItem(tr("sine"));
	connect(&m_nodeTypeModel, SIGNAL(dataChanged()), this, SLOT(chNodeType()));
	m_nodeType.setModel(&m_nodeTypeModel);
	auto curNode = instrument->m_model.m_nodes[m_model_i];
	m_nodeTypeModel.setValue(
		m_nodeTypeModel.findText(QString::fromStdString(curNode->name()))
	); //=>call to this->chNodeType
}

void HPView::chNodeType() {
	string nodeType = m_nodeTypeModel.currentText().toStdString();
	auto modelNode = m_instrument->m_model.m_nodes[m_model_i];
	if (modelNode->name() != nodeType) {
		m_instrument->chNodeType(nodeType, m_model_i);
		modelNode = m_instrument->m_model.m_nodes[m_model_i];
	}
	if (m_curNode != nullptr) {
		m_curNode->hide();
	}
	if (nodeType == "noise") {
		m_curNode = &m_noise;
		m_noise.setModel(static_pointer_cast<HPModel::Noise>(modelNode));
	}
	else if (nodeType == "shapes") {
		m_curNode = &m_shapes;
		m_shapes.setModel(static_pointer_cast<HPModel::Shapes>(modelNode));
	}
	else if (nodeType == "sine") {
		m_curNode = &m_sine;
		m_sine.setModel(static_pointer_cast<HPModel::Sine>(modelNode));
	}
	else {
		throw invalid_argument("view implementation missing for HyperPipe \"" + modelNode->name() + "\"");
	}
	m_curNode->show();
}

HPNodeView::~HPNodeView()
{
}

void HPNodeView::hide() {
	for (auto widget : m_widgets) {
		widget->hide();
	}
}

void HPNodeView::moveRel(int x, int y) {
	for (auto widget : m_widgets) {
		int origX = widget->pos().x(), origY = widget->pos().y();
		widget->move(origX + x, origY + y);
	}
}

void HPNodeView::show() {
	for (auto widget : m_widgets) {
		widget->show();
	}
}

HPNoiseView::HPNoiseView(HPView* view, HyperPipe* instrument) :
		m_spike(view, "spike")
{
	m_widgets.emplace_back(&m_spike);
}
string HPNoiseView::name() {
	return "noise";
}

void HPNoiseView::setModel(shared_ptr<HPModel::Noise> model) {
	m_spike.setModel(model->m_spike.get());
}

HPSineView::HPSineView(HPView* view, HyperPipe* instrument) :
		m_sawify(view, "sawify")
{
	m_widgets.emplace_back(&m_sawify);
}
string HPSineView::name() {
	return "sine";
}

void HPSineView::setModel(shared_ptr<HPModel::Sine> model) {
	m_sawify.setModel(model->m_sawify.get());
}

HPShapesView::HPShapesView(HPView* view, HyperPipe* instrument) :
		m_shape(view, "shape"),
		m_jitter(view, "jitter")
{
	m_widgets.emplace_back(&m_shape);
	m_widgets.emplace_back(&m_jitter);
	m_jitter.move(40, 0);
}
string HPShapesView::name() {
	return "shapes";
}

void HPShapesView::setModel(shared_ptr<HPModel::Shapes> model) {
	m_shape.setModel(model->m_shape.get());
	m_jitter.setModel(model->m_jitter.get());
}

HPView::~HPView()
{
}

} // namespace lmms::gui::hyperpipe
