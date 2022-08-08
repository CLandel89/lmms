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
void HPModel::prepend(shared_ptr<Node> node, int model_i) {
	vector<shared_ptr<Node>> recreation;
	int i = 0;
	for (auto mnode : m_nodes) {
		if (i == model_i) {
			recreation.emplace_back(node);
		}
		recreation.emplace_back(mnode);
		i++;
	}
	m_nodes = recreation;
}
void HPModel::append(shared_ptr<Node> node, int model_i) {
	vector<shared_ptr<Node>> recreation;
	int i = 0;
	for (auto mnode : m_nodes) {
		recreation.emplace_back(mnode);
		if (i == model_i) {
			recreation.emplace_back(node);
		}
		i++;
	}
	m_nodes = recreation;
}
void HPModel::remove(int model_i) {
	vector<shared_ptr<Node>> recreation;
	for (int i = 0; i < size(); i++) {
		auto mnode = m_nodes[i];
		if (i == model_i) {
			continue;
		}
		recreation.emplace_back(mnode);
	}
	m_nodes = recreation;
}
int HPModel::size() {
	return m_nodes.size();
}

HPModel::Node::Node(Instrument* instrument) :
		m_pipe(make_shared<IntModel>(0, 0, 99, instrument, QString("pipe")))
{
}

} // namespace lmms::hyperpipe
