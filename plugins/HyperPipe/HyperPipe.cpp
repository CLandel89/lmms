/*
 * HyperPipe.cpp - implementation of class HyperPipe; C export
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

extern "C" {
	Plugin::Descriptor PLUGIN_EXPORT hyperpipe_plugin_descriptor = {
		LMMS_STRINGIFY(PLUGIN_NAME),
		"HyperPipe",
		QT_TRANSLATE_NOOP("PluginBrowser", "synth with arbitrary possibilities"),
		"Christian Landel",
		0x0110,
		Plugin::Instrument,
		new PluginPixmapLoader("logo"),
		nullptr,
		nullptr,
	};
	PLUGIN_EXPORT Plugin* lmms_plugin_main(Model* model, void*) {
		return new HyperPipe(static_cast<InstrumentTrack*>(model));
	}
}

HyperPipe::HyperPipe(InstrumentTrack* instrument_track) :
		Instrument(instrument_track, &hyperpipe_plugin_descriptor),
		m_model(this)
{
}

HyperPipe::~HyperPipe() {
}

QString HyperPipe::nodeName() const {
	return hyperpipe_plugin_descriptor.name;
}

void HyperPipe::playNote(NotePlayHandle* nph, sampleFrame* working_buffer)
{
	if (nph->totalFramesPlayed() == 0 || nph->m_pluginData == nullptr) {
		if (nph->m_pluginData != nullptr) {
			delete static_cast<HPSynth*>(nph->m_pluginData);
		}
		nph->m_pluginData = new HPSynth(this, nph, &m_model);
	}
	HPSynth *synth = static_cast<HPSynth*>(nph->m_pluginData);
	const fpp_t frames = nph->framesLeftForCurrentPeriod();
	const f_cnt_t offset = nph->noteOffset();
	for (size_t i = 0; i < frames; i++) {
		working_buffer[offset + i] = synth->processFrame(
			nph->frequency(),
			Engine::audioEngine()->processingSampleRate()
		);
	}
	applyFadeIn(working_buffer, nph);
	applyRelease(working_buffer, nph);
	instrumentTrack()->processAudioBuffer(working_buffer, frames + offset, nph);
}

void HyperPipe::deleteNotePluginData(NotePlayHandle* nph) {
	delete static_cast<HPSynth*>(nph->m_pluginData);
}

void HyperPipe::saveSettings (QDomDocument& doc, QDomElement& parent)
{
}

void HyperPipe::loadSettings (const QDomElement& preset)
{
}

gui::PluginView* HyperPipe::instantiateView(QWidget* parent) {
	return new HPView(this, parent);
}

void HyperPipe::chNodeType(string nodeType, size_t model_i)
{
	if (nodeType == m_model.m_nodes[model_i]->name()) {
		return;
	}
	if (nodeType == "noise") {
		m_model.m_nodes[model_i] = make_shared<HPModel::Noise>(this);
	}
	else if (nodeType == "shapes") {
		m_model.m_nodes[model_i] = make_shared<HPModel::Shapes>(this);
	}
	else if (nodeType == "sine") {
		m_model.m_nodes[model_i] = make_shared<HPModel::Sine>(this);
	}
	else {
		throw invalid_argument("model implementation missing for HyperPipe \"" + nodeType + "\"");
	}
}

} // namespace lmms::hyperpipe
