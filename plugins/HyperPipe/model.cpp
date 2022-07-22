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

namespace lmms
{

HyperPipeModel::HyperPipeModel(Instrument* instrument) {
	m_nodes.emplace_back(make_shared<HyperPipeModel::Shapes>(instrument));
}

HyperPipeModel::Noise::Noise(Instrument* instrument) :
		m_spike(make_shared<FloatModel>(12.0f, 0.0f, 20.0f, 0.1f, instrument, tr("spike")))
{
	m_fmodels.emplace_back(m_spike);
}

shared_ptr<HyperPipeNode> HyperPipeModel::Noise::instantiate(shared_ptr<HyperPipeModel::Node> self, Instrument* instrument) {
	return make_shared<HyperPipeNoise>(
		static_pointer_cast<HyperPipeModel::Noise>(self),
		instrument
	);
}

string HyperPipeModel::Noise::name() {
	return "noise";
}

HyperPipeModel::Sine::Sine(Instrument* instrument) :
		m_sawify(make_shared<FloatModel>(0.0f, 0.0f, 1.0f, 0.01f, instrument, tr("sawify")))
{
	m_fmodels.emplace_back(m_sawify);
}

shared_ptr<HyperPipeNode> HyperPipeModel::Sine::instantiate(shared_ptr<HyperPipeModel::Node> self) {
	return make_shared<HyperPipeSine>(
		static_pointer_cast<HyperPipeModel::Sine>(self)
	);
}

string HyperPipeModel::Sine::name() {
	return "sine";
}

HyperPipeModel::Shapes::Shapes(Instrument* instrument) :
		m_shape(make_shared<FloatModel>(0.0f, -3.0f, 3.0f, 0.01f, instrument, tr("shape"))),
		m_jitter(make_shared<FloatModel>(0.0f, -3.0f, 3.0f, 0.01f, instrument, tr("jitter")))
{
	m_fmodels.emplace_back(m_shape);
	m_fmodels.emplace_back(m_jitter);
}

shared_ptr<HyperPipeNode> HyperPipeModel::Shapes::instantiate(shared_ptr<HyperPipeModel::Node> self) {
	return make_shared<HyperPipeShapes>(
		static_pointer_cast<HyperPipeModel::Shapes>(self)
	);
}

string HyperPipeModel::Shapes::name() {
	return "shapes";
}

} // namespace lmms