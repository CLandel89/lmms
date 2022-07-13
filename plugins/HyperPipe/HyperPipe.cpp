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
#include "Engine.h"
#include "InstrumentTrack.h"
#include "Knob.h"
#include "NotePlayHandle.h"
#include "Oscillator.h"
#include "PixmapButton.h"

#include "embed.h"
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


HyperPipe::HyperPipe (InstrumentTrack *_instrument_track) :
	    Instrument(_instrument_track, &hyperpipe_plugin_descriptor)
{
}

HyperPipe::~HyperPipe() {}

QString HyperPipe::nodeName () const {
	return hyperpipe_plugin_descriptor.name;
}

void HyperPipe::playNote (NotePlayHandle *_n, sampleFrame *_working_buffer)
{
	if(_n->totalFramesPlayed() == 0 || _n->m_pluginData == nullptr) {
        if (_n->m_pluginData != nullptr)
	        delete static_cast<HyperPipeSynth*>(_n->m_pluginData);
        _n->m_pluginData = new HyperPipeSynth(this, _n);
	}
    HyperPipeSynth *synth = static_cast<HyperPipeSynth*>(_n->m_pluginData);

	const fpp_t frames = _n->framesLeftForCurrentPeriod();
	const f_cnt_t offset = _n->noteOffset();

    float ph_div_sample = _n->frequency() / Engine::audioEngine()->processingSampleRate();
    for (size_t i=0; i<frames; i++) {
        synth->m_ph += ph_div_sample;
        synth->m_ph -= floor(synth->m_ph);
		float l = synth->m_ph<=0.5 ? 1.0 : -1.0;
		float r = synth->m_ph<=0.6 ? 1.0 : -1.0;
        _working_buffer[offset+i] = {l, r};
    }

	applyFadeIn(_working_buffer, _n);
	applyRelease(_working_buffer, _n);
	instrumentTrack()->processAudioBuffer(_working_buffer, frames + offset, _n);
}

void HyperPipe::deleteNotePluginData (NotePlayHandle *_n)
{
	delete static_cast<HyperPipeSynth*>(_n->m_pluginData);
}

gui::PluginView* HyperPipe::instantiateView (QWidget *_parent) {
	return new gui::HyperPipeView(this, _parent);
}


extern "C" {
    PLUGIN_EXPORT Plugin* lmms_plugin_main(Model* model, void*) {
        return new HyperPipe(static_cast<InstrumentTrack *>(model));
    }
}

} // namespace lmms