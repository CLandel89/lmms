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

#include <QMouseEvent>
#include <QPainter>
#include <QTimer>
#include <QWheelEvent>

namespace lmms::gui::hyperpipe
{

inline const int VW = 250, VH = 250;

HPView::MapWidget::MapWidget(HPView* parent) :
		QWidget(parent),
		m_parent(parent),
		m_timer(new QTimer(this))
{
	auto &definitions = m_parent->m_instrument->m_definitions;
	float ph = 0;
	for (auto &definition : definitions) {
		QColor c;
		float ph3 = 3 * fmod(ph, 1.0f / 3);
		if (ph < 1.0 / 3) {
			c = QColor((1 - ph3) * 255, ph3 * 255, 0);
		}
		else if (ph < 2.0 / 3) {
			c = QColor(0, (1 - ph3) * 255, ph3 * 255);
		}
		else {
			c = QColor(ph3 * 255, 0, (1 - ph3) * 255);
		}
		m_colors[definition.first] = c;
		ph += 1.0f / definitions.size();
	}
	// the LcdSpinBoxes don't signal value changes from the scroll wheel, so...
	connect(m_timer, SIGNAL(timeout()), this, SLOT(sl_update()));
	m_timer->start(2000);
}
void HPView::MapWidget::mousePressEvent(QMouseEvent* ev) {
	if (m_model == nullptr) { return; }
	m_parent->setModel_i(
		ev->x() * m_model->m_nodes.size() / geometry().width()
	);
}
void HPView::MapWidget::paintEvent(QPaintEvent* ev) {
	QWidget::paintEvent(ev);
	if (m_model == nullptr) { return; }
	QPainter painter(this);
	float w = geometry().width(), h = geometry().height();
	float nw = w / m_model->m_nodes.size();
	int i = m_parent->model_i();
	painter.fillRect(0, 0, w, h, QColor(0, 0, 0));
	painter.fillRect(i * nw, 0, nw, h, QColor(128, 128, 128));
	map<int, int> pipe2y;
	float nh;
	{
		map<int, int> pipe2y_prep;
		for (auto &node : m_model->m_nodes) {
			pipe2y_prep[node->m_pipe.value()] = -1;
		}
		nh = h / pipe2y_prep.size();
		pipe2y = pipe2y_prep;
		float y = 0;
		for (auto &entry : pipe2y_prep) {
			pipe2y[entry.first] = y;
			y += nh;
		}
	}
	int ni = 0;
	for (auto &node : m_model->m_nodes) {
		int y = pipe2y[node->m_pipe.value()];
		QColor c = m_colors[node->name()];
		painter.fillRect(ni * nw, y, nw, h / pipe2y.size(), c);
		for (auto &argument : node->m_arguments) {
			int arg_ni;
			for (arg_ni = ni - 1; arg_ni >= 0; arg_ni--) {
				if (m_model->m_nodes[arg_ni]->m_pipe.value() == argument->value()) {
					break;
				}
			}
			if (arg_ni < 0) {
				//argument is (currently) invalid
				continue;
			}
			y = pipe2y[argument->value()];
			painter.fillRect(ni * nw, y, nw / 2, nh, QColor(255, 255, 255));
			painter.fillRect(
				(arg_ni + 1) * nw,
				y + nh / 2,
				(ni - arg_ni - 1) * nw,
				1,
				QColor(255, 255, 255));
		}
		ni++;
	}
	painter.fillRect(i * nw, 0, nw, 1, QColor(128, 128, 128));
	painter.fillRect(i * nw, h - 1, nw, 1, QColor(128, 128, 128));
}
void HPView::MapWidget::wheelEvent(QWheelEvent* ev) {
	if (m_model == nullptr) { return; }
	if (ev->angleDelta().y() > 0 && m_parent->model_i() > 0) {
		m_parent->setModel_i(m_parent->model_i() - 1);
		ev->accept();
	}
	if (ev->angleDelta().y() < 0 && m_parent->model_i() < m_model->m_nodes.size() - 1) {
		m_parent->setModel_i(m_parent->model_i() + 1);
		ev->accept();
	}
}
void HPView::MapWidget::sl_update() {
	update();
}

HPView::HPView(HPInstrument* instrument, QWidget* parent) :
		InstrumentView(instrument, parent),
		m_instrument(instrument),
		m_nodeType(new ComboBox(this, "node type")),
		m_pipe(new LcdSpinBox(2, this, "pipe")),
		m_prev(new PixmapButton(this)),
		m_next(new PixmapButton(this)),
		m_moveUp(new PixmapButton(this)),
		m_prepend(new PixmapButton(this)),
		m_delete(new PixmapButton(this)),
		m_append(new PixmapButton(this)),
		m_moveDown(new PixmapButton(this)),
		m_arguments(this, instrument),
		m_map(new MapWidget(this))
{
	m_map->m_model = &m_instrument->m_model;
	m_map->move(0, VH - 50);
	m_map->resize(VW, 50);

	auto curNode = instrument->m_model.m_nodes[m_model_i].get();

	// node view
	for (auto& definition : instrument->m_definitions) {
		m_nodeViews[definition.second->name()] =
			definition.second->instantiateView(this);
		m_nodeViews[definition.second->name()]->moveRel(0, 60);
		m_nodeViews[definition.second->name()]->hide();
	}
	// node type combo box
	m_nodeType->move(0, 30);
	for (auto& definition : instrument->m_definitions) {
		m_nodeTypeModel.addItem(QString::fromStdString(definition.first));
	}
	connect(&m_nodeTypeModel, SIGNAL(dataChanged()), this, SLOT(sl_chNodeType()));
	m_nodeType->setModel(&m_nodeTypeModel);
	m_nodeTypeModel.setValue(
		m_nodeTypeModel.findText(QString::fromStdString(curNode->name()))
	); //=>call to this->s_chNodeType
	// node pipe number
	m_pipe->move(120, 30);
	m_pipe->setModel(&curNode->m_pipe);

	// node move/create/delete buttons
	m_prev->setActiveGraphic(PLUGIN_NAME::getIconPixmap("prev"));
	m_prev->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("prev"));
	m_prev->move(10, 5);
	connect(m_prev, SIGNAL(clicked()), this, SLOT(sl_prev()));
	m_next->setActiveGraphic(PLUGIN_NAME::getIconPixmap("next"));
	m_next->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("next"));
	m_next->move(40, 5);
	connect(m_next, SIGNAL(clicked()), this, SLOT(sl_next()));
	m_moveUp->setActiveGraphic(PLUGIN_NAME::getIconPixmap("moveUp"));
	m_moveUp->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("moveUp"));
	m_moveUp->move(80, 5);
	connect(m_moveUp, SIGNAL(clicked()), this, SLOT(sl_moveUp()));
	m_prepend->setActiveGraphic(PLUGIN_NAME::getIconPixmap("prepend"));
	m_prepend->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("prepend"));
	m_prepend->move(110, 5);
	connect(m_prepend, SIGNAL(clicked()), this, SLOT(sl_prepend()));
	m_delete->setActiveGraphic(PLUGIN_NAME::getIconPixmap("delete"));
	m_delete->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("delete"));
	m_delete->move(140, 5);
	connect(m_delete, SIGNAL(clicked()), this, SLOT(sl_delete()));
	m_append->setActiveGraphic(PLUGIN_NAME::getIconPixmap("append"));
	m_append->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("append"));
	m_append->move(170, 5);
	connect(m_append, SIGNAL(clicked()), this, SLOT(sl_append()));
	m_moveDown->setActiveGraphic(PLUGIN_NAME::getIconPixmap("moveDown"));
	m_moveDown->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("moveDown"));
	m_moveDown->move(200, 5);
	connect(m_moveDown, SIGNAL(clicked()), this, SLOT(sl_moveDown()));
}

HPView::~HPView() {
	m_destructing = true;
	m_map->m_model = nullptr;
}

void HPView::setModel_i(int i) {
	m_model_i = i;
	updateNodeView();
	updateWidgets();
	update();
}

void HPView::updateWidgets() {
	m_map->update();
}

void HPView::sl_chNodeType() {
	if (m_destructing) { return; }
	string nodeType = m_nodeTypeModel.currentText().toStdString();
	auto modelNode = m_instrument->m_model.m_nodes[m_model_i].get();
	if (modelNode->name() != nodeType) {
		m_instrument->chNodeType(nodeType, m_model_i);
	}
	updateNodeView();
}

void HPView::updateNodeView() {
	auto modelNode = m_instrument->m_model.m_nodes[m_model_i].get();
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
	m_curNode = m_nodeViews[nodeType].get();
	m_curNode->setModel(modelNode);
	m_curNode->show();
	m_pipe->setModel(&modelNode->m_pipe);
	m_arguments.setModel(modelNode);
	updateWidgets();
}

void HPView::sl_prev() {
	if (m_destructing) { return; }
	if (m_model_i <= 0) { return; }
	m_model_i--;
	updateNodeView();
}

void HPView::sl_next() {
	if (m_destructing) { return; }
	if (m_model_i >= m_instrument->m_model.m_nodes.size() - 1) { return; }
	m_model_i++;
	updateNodeView();
}

void HPView::sl_moveUp() {
	if (m_destructing) { return; }
	if (m_model_i <= 0) { return; }
	auto &nodes = m_instrument->m_model.m_nodes;
	swap(nodes[m_model_i], nodes[m_model_i - 1]);
	m_model_i--;
	updateNodeView();
}

void HPView::sl_prepend() {
	if (m_destructing) { return; }
	auto mnode = m_instrument->m_definitions[HPDefinitionBase::DEFAULT_TYPE]->newNode();
	auto &nodes = m_instrument->m_model.m_nodes;
	mnode->m_pipe.setValue(nodes[m_model_i]->m_pipe.value());
	nodes.insert(nodes.begin() + m_model_i, std::move(mnode));
	updateNodeView();
}

void HPView::sl_delete() {
	if (m_destructing) { return; }
	auto &nodes = m_instrument->m_model.m_nodes;
	if (nodes.size() <= 1) { return; }
	nodes.erase(nodes.begin() + m_model_i);
	if (m_model_i >= nodes.size()) { m_model_i--; }
	updateNodeView();
}

void HPView::sl_append() {
	if (m_destructing) { return; }
	auto mnode = m_instrument->m_definitions[HPDefinitionBase::DEFAULT_TYPE]->newNode();
	auto &nodes = m_instrument->m_model.m_nodes;
	mnode->m_pipe.setValue(nodes[m_model_i]->m_pipe.value());
	nodes.insert(nodes.begin() + m_model_i + 1, std::move(mnode));
	m_model_i++;
	updateNodeView();
}

void HPView::sl_moveDown() {
	if (m_destructing) { return; }
	auto &nodes = m_instrument->m_model.m_nodes;
	if (m_model_i >= nodes.size() - 1) { return; }
	swap(nodes[m_model_i], nodes[m_model_i + 1]);
	m_model_i++;
	updateNodeView();
}

HPVArguments::HPVArguments(HPView* view, HPInstrument* instrument) :
		m_instrument(instrument),
		m_view(view),
		m_left(new PixmapButton(view)),
		m_right(new PixmapButton(view)),
		m_add(new PixmapButton(view)),
		m_delete(new PixmapButton(view))
{
	const int Y = VH - 80;
	m_add->setActiveGraphic(PLUGIN_NAME::getIconPixmap("plus"));
	m_add->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("plus"));
	m_add->move(5, Y);
	connect(m_add, SIGNAL(clicked()), this, SLOT(sl_add()));
	m_delete->setActiveGraphic(PLUGIN_NAME::getIconPixmap("minus"));
	m_delete->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("minus"));
	m_delete->move(30, Y);
	connect(m_delete, SIGNAL(clicked()), this, SLOT(sl_delete()));
	const int argw = 35;
	const int nShown = 4;
	m_left->setActiveGraphic(PLUGIN_NAME::getIconPixmap("left"));
	m_left->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("left"));
	m_left->move(VW - 2 * 25 - nShown * argw, Y);
	connect(m_left, SIGNAL(clicked()), this, SLOT(sl_left()));
	for (int li = 0; li < nShown; li++) {
		auto pipe = new LcdSpinBox(2, view, "argument");
		pipe->move(VW + (-nShown + li) * argw - 25, Y);
		m_pipes.emplace_back(pipe);
	}
	m_right->setActiveGraphic(PLUGIN_NAME::getIconPixmap("right"));
	m_right->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("right"));
	m_right->move(VW - 25, Y);
	connect(m_right, SIGNAL(clicked()), this, SLOT(sl_right()));
}

HPVArguments::~HPVArguments() {
	m_destructing = true;
}

void HPVArguments::setModel(HPModel::Node* model) {
	if (model == m_model) {
		return;
	}
	m_pos = 0;
	m_model = model;
	update();
}

void HPVArguments::update() {
	if (m_model == nullptr) {
		for (auto& pipe : m_pipes) {
			pipe->hide();
		}
		return;
	}
	const int maxPos = static_cast<int>(m_model->m_arguments.size()) - m_pipes.size();
	if (m_pos > maxPos) {
		m_pos = maxPos;
	}
	if (m_pos < 0) {
		m_pos = 0;
	}
	for (int li = 0; li < m_pipes.size(); li++) {
		auto ai = m_pos + li;
		if (m_model->m_arguments.size() > ai) {
			m_pipes[li]->show();
			m_pipes[li]->setModel(m_model->m_arguments[ai].get());
		}
		else {
			m_pipes[li]->hide();
		}
	}
	m_view->updateWidgets();
}

void HPVArguments::sl_left() {
	if (m_destructing) { return; }
	//conditions checked and corrected in update()
	m_pos--;
	update();
}

void HPVArguments::sl_right() {
	if (m_destructing) { return; }
	m_pos++;
	update();
}

void HPVArguments::sl_add() {
	if (m_destructing) { return; }
	if (m_model == nullptr) { return; }
	if (m_instrument->m_definitions[m_model->name()]->forbidsArguments()) { return; }
	const auto ai = m_model->m_arguments.size();
	m_model->m_arguments.emplace_back(
		HPModel::newArgument(m_instrument, ai)
	);
	m_pos++;
	update();
}

void HPVArguments::sl_delete() {
	if (m_destructing) { return; }
	if (m_model == nullptr) { return; }
	if (m_model->m_arguments.size() <= 0) { return; }
	m_model->m_arguments.pop_back();
	update();
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

} // namespace lmms::gui::hyperpipe
