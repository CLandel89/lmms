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

#include <limits>

namespace {
	const float inf = std::numeric_limits<float>::infinity();
}

namespace lmms::hyperpipe
{

void HPNode::resetState() {}

HPOsc::HPOsc(HPModel* model, int model_i, shared_ptr<HPOscModel> nmodel) :
		m_nmodel(nmodel),
		m_arguments(model->instantiateArguments(model_i))
{}

float HPOsc::processFrame(Params p) {
	float ph = 0;
	if (m_arguments.empty()) {
		// synthesize shape as is
		ph = p.ph;
	}
	else {
		// mix audio of all "arguments"
		for (auto &argument : m_arguments) {
			ph += argument->processFrame(p);
		}
		// amp the audio signal, if multiple "arguments"
		ph /= m_arguments.size();
		// translate the audio signal into the "phase" range (0...1)
		ph = (ph + 1) / 2;
	}
	ph += m_nmodel->m_ph.value() / 360.0f;
	ph = hpposmodf(ph);
	return shape(ph);
}

void HPOsc::resetState() {
	HPNode::resetState();
	for (auto &argument : m_arguments) {
		argument->resetState();
	}
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
	float f = m_lastNode->processFrame(HPNode::Params {
		freq: freq,
		freqMod: freq,
		srate: srate,
		ph: m_ph,
	});
	if (f > 100000)         f = 100000.0;
	else if (f < -100000)   f = -100000.0;
	else if (!isfinite(f))  f = 0.0;
	m_ph += freq / srate;
	m_ph = hpposmodf(m_ph);
	return {f, f};
}

} // namespace lmms::hyperpipe
