/*
 * model.cpp - implementation of the data (preset) model
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

namespace lmms::hyperpipe
{

HPModel::HPModel(Instrument* instrument) {
	m_nodes.emplace_back(make_shared<HPModel::Shapes>(instrument));
}

HPModel::Noise::Noise(Instrument* instrument) :
		m_spike(make_shared<FloatModel>(4.0f, 0.0f, 20.0f, 0.1f, instrument, tr("spike")))
{
	m_fmodels.emplace_back(m_spike);
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
		m_sawify(make_shared<FloatModel>(0.0f, 0.0f, 1.0f, 0.01f, instrument, tr("sawify")))
{
	m_fmodels.emplace_back(m_sawify);
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
		m_shape(make_shared<FloatModel>(0.0f, -3.0f, 3.0f, 0.01f, instrument, tr("shape"))),
		m_jitter(make_shared<FloatModel>(0.0f, -3.0f, 3.0f, 0.01f, instrument, tr("jitter")))
{
	m_fmodels.emplace_back(m_shape);
	m_fmodels.emplace_back(m_jitter);
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
