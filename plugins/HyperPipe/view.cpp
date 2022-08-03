/*
	view.cpp - implementation of the user interface

	HyperPipe - synth with arbitrary possibilities

	Copyright (c) 2022 Christian Landel

	This file is part of LMMS - https://lmms.io

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public
	License as published by the Free Software Foundation; either
	version 2 of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	General Public License for more details.

	You should have received a copy of the GNU General Public
	License along with this program (see COPYING); if not, write to the
	Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
	Boston, MA 02110-1301 USA.
*/

#include "HyperPipe.h"

namespace lmms::gui::hyperpipe
{

HPView::HPView(HPInstrument* instrument, QWidget* parent) :
		InstrumentView(instrument, parent),
		m_noise(this, instrument),
		m_shapes(this, instrument),
		m_sine(this, instrument),
		m_instrument(instrument),
		m_nodeType(this, "node type"),
		m_pipe(2, this, "pipe"),
		m_prev(this),
		m_next(this),
		m_moveUp(this),
		m_prepend(this),
		m_delete(this),
		m_append(this),
		m_moveDown(this)
{
	auto curNode = instrument->m_model.m_nodes[m_model_i];

	// node view
	m_noise.moveRel(0, 60);
	m_noise.hide();
	m_shapes.moveRel(0, 60);
	m_shapes.hide();
	m_sine.moveRel(0, 60);
	m_sine.hide();

	// node type combo box
	m_nodeType.move(0, 30);
	m_nodeTypeModel.addItem(tr("noise"));
	m_nodeTypeModel.addItem(tr("shapes"));
	m_nodeTypeModel.addItem(tr("sine"));
	connect(&m_nodeTypeModel, SIGNAL(dataChanged()), this, SLOT(sl_chNodeType()));
	m_nodeType.setModel(&m_nodeTypeModel);
	m_nodeTypeModel.setValue(
		m_nodeTypeModel.findText(QString::fromStdString(curNode->name()))
	); //=>call to this->s_chNodeType

	m_pipe.move(120, 30);
	m_pipe.setModel(curNode->m_pipe.get());

	// node move/create/delete buttons
	m_prev.setActiveGraphic(PLUGIN_NAME::getIconPixmap("prev"));
	m_prev.setInactiveGraphic(PLUGIN_NAME::getIconPixmap("prev"));
	m_prev.move(10, 5);
	connect(&m_prev, SIGNAL(clicked()), this, SLOT(sl_prev()));
	m_next.setActiveGraphic(PLUGIN_NAME::getIconPixmap("next"));
	m_next.setInactiveGraphic(PLUGIN_NAME::getIconPixmap("next"));
	m_next.move(40, 5);
	connect(&m_next, SIGNAL(clicked()), this, SLOT(sl_next()));
	m_moveUp.setActiveGraphic(PLUGIN_NAME::getIconPixmap("moveUp"));
	m_moveUp.setInactiveGraphic(PLUGIN_NAME::getIconPixmap("moveUp"));
	m_moveUp.move(80, 5);
	connect(&m_moveUp, SIGNAL(clicked()), this, SLOT(sl_moveUp()));
	m_prepend.setActiveGraphic(PLUGIN_NAME::getIconPixmap("prepend"));
	m_prepend.setInactiveGraphic(PLUGIN_NAME::getIconPixmap("prepend"));
	m_prepend.move(110, 5);
	connect(&m_prepend, SIGNAL(clicked()), this, SLOT(sl_prepend()));
	m_delete.setActiveGraphic(PLUGIN_NAME::getIconPixmap("delete"));
	m_delete.setInactiveGraphic(PLUGIN_NAME::getIconPixmap("delete"));
	m_delete.move(140, 5);
	connect(&m_delete, SIGNAL(clicked()), this, SLOT(sl_delete()));
	m_append.setActiveGraphic(PLUGIN_NAME::getIconPixmap("append"));
	m_append.setInactiveGraphic(PLUGIN_NAME::getIconPixmap("append"));
	m_append.move(170, 5);
	connect(&m_append, SIGNAL(clicked()), this, SLOT(sl_append()));
	m_moveDown.setActiveGraphic(PLUGIN_NAME::getIconPixmap("moveDown"));
	m_moveDown.setInactiveGraphic(PLUGIN_NAME::getIconPixmap("moveDown"));
	m_moveDown.move(200, 5);
	connect(&m_moveDown, SIGNAL(clicked()), this, SLOT(sl_moveDown()));
}

void HPView::sl_chNodeType() {
	string nodeType = m_nodeTypeModel.currentText().toStdString();
	auto modelNode = m_instrument->m_model.m_nodes[m_model_i];
	if (modelNode->name() != nodeType) {
		m_instrument->chNodeType(nodeType, m_model_i);
		modelNode = m_instrument->m_model.m_nodes[m_model_i];
	}
	updateNodeView();
}

void HPView::updateNodeView() {
	auto modelNode = m_instrument->m_model.m_nodes[m_model_i];
	string nodeType = modelNode->name();
	if (nodeType != m_nodeTypeModel.currentText().toStdString()) {
		//combo box needs update
		m_nodeTypeModel.setValue(
			m_nodeTypeModel.findText(QString::fromStdString(nodeType))
		);
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
	m_pipe.setModel(modelNode->m_pipe.get());
}

void HPView::sl_prev() {
	if (m_model_i <= 0) { return; }
	m_model_i--;
	updateNodeView();
}

void HPView::sl_next() {
	if (m_model_i >= m_instrument->m_model.size() - 1) { return; }
	m_model_i++;
	updateNodeView();
}

void HPView::sl_moveUp() {
	if (m_model_i <= 0) { return; }
	auto &nodes = m_instrument->m_model.m_nodes;
	swap(nodes[m_model_i], nodes[m_model_i - 1]);
	m_model_i--;
	updateNodeView();
}

void HPView::sl_prepend() {
	auto mnode = make_shared<HPModel::Shapes>(m_instrument);
	m_instrument->m_model.prepend(mnode, m_model_i);
	updateNodeView();
}

void HPView::sl_delete() {
	if (m_instrument->m_model.size() <= 1) { return; }
	m_instrument->m_model.remove(m_model_i);
	if (m_model_i >= m_instrument->m_model.size()) { m_model_i--; }
	updateNodeView();
}

void HPView::sl_append() {
	auto mnode = make_shared<HPModel::Shapes>(m_instrument);
	m_instrument->m_model.append(mnode, m_model_i);
	m_model_i++;
	updateNodeView();
}

void HPView::sl_moveDown() {
	if (m_model_i >= m_instrument->m_model.size() - 1) { return; }
	auto &nodes = m_instrument->m_model.m_nodes;
	swap(nodes[m_model_i], nodes[m_model_i + 1]);
	m_model_i++;
	updateNodeView();
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

HPNoiseView::HPNoiseView(HPView* view, HPInstrument* instrument) :
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

HPSineView::HPSineView(HPView* view, HPInstrument* instrument) :
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

HPShapesView::HPShapesView(HPView* view, HPInstrument* instrument) :
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

} // namespace lmms::gui::hyperpipe
