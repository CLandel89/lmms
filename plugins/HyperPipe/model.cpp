/*
	model.cpp - implementation of the data (preset) model

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

namespace lmms::hyperpipe
{

HPModel::HPModel(HPInstrument* instrument) {
	m_nodes.emplace_back(
		instrument->m_definitions[HPDefinitionBase::DEFAULT_TYPE]->newNode()
	);
}

unique_ptr<HPNode> HPModel::instantiatePrev(int i) {
	// the "current" node looks backwards for any "previous" node
	for (int j = i - 1; j >= 0; j--) {
		if (m_nodes[i]->prevPipe() == m_nodes[j]->m_pipe.value()) {
			return m_nodes[j]->instantiate(this, j);
		}
	}
	return nullptr;
}

vector<unique_ptr<HPNode>> HPModel::instantiateArguments(int i)
{
	// we need to track which arguments are valid and which must be left out
	vector<unique_ptr<HPNode>> result;
	for (int n = 0; n < m_nodes[i]->m_arguments.size(); n++) {
		result.emplace_back(nullptr);
	}
	// the "current" node looks backwards for any associated arguments
	for (int j = i - 1; j >= 0; j--) {
		for (int ai = 0; ai < m_nodes[i]->m_arguments.size(); ai++) {
			if (result[ai] == nullptr && // only set at first encounter
					m_nodes[i]->m_arguments[ai]->value() == m_nodes[j]->m_pipe.value())
			{
				result[ai] = m_nodes[j]->instantiate(this, j);
			}
		}
	}
	// clean up, also going backwards
	for (int ai = result.size() - 1; ai >= 0; ai--) {
		if (result[ai] == nullptr) {
			result.erase(result.begin() + ai);
		}
	}
	return result;
}

unique_ptr<IntModel> HPModel::newArgument(Instrument* instrument, int i) {
	return make_unique<IntModel>(0, 0, 99, instrument, QString("argument" + i));
}

HPModel::Node::Node(Instrument* instrument) :
		m_pipe(0, 0, 99, instrument, QString("pipe")),
		m_customPrev(-1, -1, 99, instrument, QString("custom prev pipe"))
{}
int HPModel::Node::prevPipe() {
	if (m_customPrev.value() == -1) {
		return m_pipe.value();
	}
	return m_customPrev.value();
}

HPOscModel::HPOscModel(Instrument* instrument) :
		Node(instrument),
		m_ph(0.0f, -360.0f, 360.0f, 0.1f, instrument, QString("phase"))
{}
void HPOscModel::load(int model_i, const QDomElement& elem) {
	QString is = "n" + QString::number(model_i);
	m_ph.loadSettings(elem, is + "_phase");
	loadImpl(model_i, elem);
}
void HPOscModel::save(int model_i, QDomDocument& doc, QDomElement& elem) {
	QString is = "n" + QString::number(model_i);
	m_ph.saveSettings(doc, elem, is + "_phase");
	saveImpl(model_i, doc, elem);
}
bool HPOscModel::usesPrev() { return false; }

} // namespace lmms::hyperpipe