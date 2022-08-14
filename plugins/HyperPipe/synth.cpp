/*
	synth.cpp - implementation of HPSynth and HPOsc

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

HPOsc::HPOsc(HPModel* model, int model_i) :
		m_arguments(model->instantiateArguments(model_i))
{}

float HPOsc::processFrame(float freq, float srate) {
	float result = shape(m_ph);
	if (m_arguments.empty()) {
		m_ph += freq / srate;
	}
	else {
		m_ph = 0;
		for (auto &argument : m_arguments) {
			m_ph += argument->processFrame(freq, srate);
		}
		m_ph /= m_arguments.size();
		m_ph = 1 - 2 * m_ph;
	}
	m_ph = absFraction(m_ph);
	return result;
}

HPSynth::HPSynth(HPInstrument* instrument, NotePlayHandle* nph, HPModel* model) :
		m_instrument(instrument),
		m_nph(nph)
{
	auto &nodes = model->m_nodes;
	if (nodes.size() > 0) {
		m_lastNode = nodes.back()->instantiate(model, nodes.size() - 1);
	}
}

array<float,2> HPSynth::processFrame(float freq, float srate) {
	if (m_lastNode == nullptr) {
		return {0.0f, 0.0f};
	}
	float f = m_lastNode->processFrame(freq, srate);
	return {f, f};
}

} // namespace lmms::hyperpipe
