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

HPModel::HPModel(Instrument* instrument) {
	m_nodes.emplace_back(make_shared<HPModel::Shapes>(instrument));
}
void HPModel::prepend(shared_ptr<Node> node, size_t model_i) {
	vector<shared_ptr<Node>> recreation;
	size_t i = 0;
	for (auto mnode : m_nodes) {
		if (i == model_i) {
			recreation.emplace_back(node);
		}
		recreation.emplace_back(mnode);
		i++;
	}
	m_nodes = recreation;
}
void HPModel::append(shared_ptr<Node> node, size_t model_i) {
	vector<shared_ptr<Node>> recreation;
	size_t i = 0;
	for (auto mnode : m_nodes) {
		recreation.emplace_back(mnode);
		if (i == model_i) {
			recreation.emplace_back(node);
		}
		i++;
	}
	m_nodes = recreation;
}
void HPModel::remove(size_t model_i) {
	vector<shared_ptr<Node>> recreation;
	for (size_t i = 0; i < size(); i++) {
		auto mnode = m_nodes[i];
		if (i == model_i) {
			continue;
		}
		recreation.emplace_back(mnode);
	}
	m_nodes = recreation;
}
size_t HPModel::size() {
	return m_nodes.size();
}

HPModel::Node::Node(Instrument* instrument) :
		m_pipe(make_shared<IntModel>(0, 0, 99, instrument, tr("pipe")))
{
}

HPModel::Noise::Noise(Instrument* instrument) :
		Node(instrument),
		m_spike(make_shared<FloatModel>(4.0f, 0.0f, 20.0f, 0.1f, instrument, tr("spike")))
{
}

unique_ptr<HPNode> HPModel::Noise::instantiate(shared_ptr<HPModel::Node> self) {
	return make_unique<HPNoise>(
		static_pointer_cast<HPModel::Noise>(self)
	);
}

string HPModel::Noise::name() {
	return "noise";
}

HPModel::Sine::Sine(Instrument* instrument) :
		Node(instrument),
		m_sawify(make_shared<FloatModel>(0.0f, 0.0f, 1.0f, 0.01f, instrument, tr("sawify")))
{
}

unique_ptr<HPNode> HPModel::Sine::instantiate(shared_ptr<HPModel::Node> self) {
	return make_unique<HPSine>(
		static_pointer_cast<HPModel::Sine>(self)
	);
}

string HPModel::Sine::name() {
	return "sine";
}

HPModel::Shapes::Shapes(Instrument* instrument) :
		Node(instrument),
		m_shape(make_shared<FloatModel>(0.0f, -3.0f, 3.0f, 0.01f, instrument, tr("shape"))),
		m_jitter(make_shared<FloatModel>(0.0f, 0.0f, 100.0f, 1.0f, instrument, tr("jitter")))
{
}

unique_ptr<HPNode> HPModel::Shapes::instantiate(shared_ptr<HPModel::Node> self) {
	return make_unique<HPShapes>(
		static_pointer_cast<HPModel::Shapes>(self)
	);
}

string HPModel::Shapes::name() {
	return "shapes";
}

} // namespace lmms::hyperpipe
