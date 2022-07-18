/*
 * HyperPipe.cpp - synth with arbitrary possibilities
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

#include "AudioEngine.h"
#include "AutomatableButton.h"
#include "debug.h"
#include "embed.h"
#include "Engine.h"
#include "InstrumentTrack.h"
#include "Knob.h"
#include "NotePlayHandle.h"
#include "Oscillator.h"
#include "PixmapButton.h"
#include "plugin_export.h"

namespace lmms
{

extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT hyperpipe_plugin_descriptor =
{
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

} // extern "C"

HyperPipe::HyperPipe (InstrumentTrack *instrument_track) :
		Instrument(instrument_track, &hyperpipe_plugin_descriptor),
		m_shape(0.0f, -3.0f, 3.0f, 0.01f, this, tr("shape")),
		m_jitter(0.0f, -3.0f, 3.0f, 0.01f, this, tr("jitter"))
{
}

HyperPipe::~HyperPipe()
{
}

QString HyperPipe::nodeName () const
{
	return hyperpipe_plugin_descriptor.name;
}

void HyperPipe::playNote (NotePlayHandle *nph, sampleFrame *working_buffer)
{
	if (nph->totalFramesPlayed() == 0 || nph->m_pluginData == nullptr)
	{
		if (nph->m_pluginData != nullptr)
		{
			delete static_cast<HyperPipeSynth*>(nph->m_pluginData);
		}
		nph->m_pluginData = new HyperPipeSynth(this, nph);
	}
	HyperPipeSynth *synth = static_cast<HyperPipeSynth*>(nph->m_pluginData);

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

void HyperPipe::deleteNotePluginData (NotePlayHandle *nph)
{
	delete static_cast<HyperPipeSynth*>(nph->m_pluginData);
}

gui::PluginView* HyperPipe::instantiateView (QWidget *parent)
{
	return new gui::HyperPipeView(this, parent);
}

extern "C"
{
	PLUGIN_EXPORT Plugin* lmms_plugin_main(Model* model, void*)
	{
		return new HyperPipe(static_cast<InstrumentTrack *>(model));
	}
}

} // namespace lmms
