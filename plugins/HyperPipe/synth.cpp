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

float HPOsc::processFrame(float freq, float srate) {
	m_ph += freq / srate;
	while (m_ph >= 1.0f) { m_ph -= 1.0f; }
	while (m_ph < 0.0f) { m_ph += 1.0f; }
	float result = shape(m_ph);
	int number = 1;
	if (m_prev != nullptr) {
		result += m_prev->processFrame(freq, srate);
		number++;
	}
	for (auto& argument : m_arguments) {
		result += argument->processFrame(freq, srate);
		number++;
	}
	return result / number;
}

HPSynth::HPSynth(HPInstrument* instrument, NotePlayHandle* nph, HPModel* model) :
		m_instrument(instrument),
		m_nph(nph)
{
	// Create synth nodes by recursing this:
	function<unique_ptr<HPNode>(int)> instantiate = [&model, &instantiate] (int i)
	{
		auto mnode = model->m_nodes[i];
		auto result = mnode->instantiate(mnode);
		unique_ptr<HPNode> prev = nullptr;
		vector<unique_ptr<HPNode>> arguments(mnode->m_arguments.size());
		//crawl upwards in the model (i.e., decrement j) and instantiate prev and arguments at first encounters
		for (int j = i - 1; j >= 0; j--) {
			auto mnode_j = model->m_nodes[j];
			if (prev == nullptr &&
					mnode->m_pipe->value() == mnode_j->m_pipe->value())
			{
				prev = instantiate(j);
			}
			for (int ai = 0; ai < mnode->m_arguments.size(); ai++) {
				if (arguments[ai] == nullptr &&
						mnode->m_arguments[ai]->value() == mnode_j->m_pipe->value())
				{
					arguments[ai] = instantiate(j);
				}
			}
		}
		if (prev != nullptr) {
			result->m_prev = move(prev);
		}
		for (auto& argument : arguments) {
			if (argument != nullptr) {
				result->m_arguments.emplace_back(move(argument));
			}
		}
		return result;
	};
	if (model->m_nodes.size() > 0) {
		m_lastNode = instantiate(model->m_nodes.size() - 1);
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
