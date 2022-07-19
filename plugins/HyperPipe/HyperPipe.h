/*
 * HyperPipe.h - declaration of class HyperPipe synth with arbitrary
 *                      possibilities
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

#pragma once

#include "AudioEngine.h"
#include "ComboBox.h"
#include "Engine.h"
#include "Instrument.h"
#include "InstrumentView.h"
#include "InstrumentTrack.h"
#include "Knob.h"
#include "lmms_math.h"
#include "NotePlayHandle.h"
#include "plugin_export.h"

namespace lmms
{

class HyperPipe;

class HyperPipeNode
{
public:
	HyperPipeNode();
	virtual ~HyperPipeNode();
	virtual float processFrame(float freq, float srate) = 0;
	virtual void updateFromUI(HyperPipe* instrument) = 0;
};

class HyperPipeOsc : public HyperPipeNode
{
public:
	HyperPipeOsc();
	virtual ~HyperPipeOsc();
	float processFrame(float freq, float srate);
private:
	virtual float shape(float ph) = 0;
	float m_ph = 0.0f;
};

class HyperPipeSine : public HyperPipeOsc
{
public:
	HyperPipeSine();
	virtual ~HyperPipeSine();
	void updateFromUI(HyperPipe* instrument);
	float m_sawify = 0.0f;
private:
	float shape(float ph);
};

class HyperPipeShapes : public HyperPipeOsc
{
public:
	HyperPipeShapes();
	virtual ~HyperPipeShapes();
	void updateFromUI(HyperPipe* instrument);
	float m_shape = 0.0f;
	float m_jitter = 0.0f;
private:
	float shape(float ph);
};

class HyperPipeNoise : public HyperPipeNode
{
public:
	HyperPipeNoise();
	virtual ~HyperPipeNoise();
	float processFrame(float freq, float srate);
	void updateFromUI(HyperPipe* instrument);
private:
	HyperPipeSine m_osc;
	float m_spike = 12.0f;
};

class HyperPipeSynth
{
	MM_OPERATORS
public:
	HyperPipeSynth(HyperPipe* parent, NotePlayHandle* nph);
	virtual ~HyperPipeSynth();
	std::array<float,2> processFrame(float freq, float srate);
private:
	HyperPipe *m_parent;
	NotePlayHandle *m_nph;
	HyperPipeShapes myOsc;
	HyperPipeNode *m_lastNode;
};

namespace gui
{
	class HyperPipeView;
}

class HyperPipe : public Instrument
{
	Q_OBJECT
public:
	HyperPipe(InstrumentTrack* track);
	virtual ~HyperPipe();
	void playNote(NotePlayHandle* nph, sampleFrame* working_buffer) override;
	void deleteNotePluginData(NotePlayHandle* nph) override;
	void saveSettings(QDomDocument& doc, QDomElement& parent) override;
	void loadSettings(const QDomElement& preset) override;
	QString nodeName() const override;
	gui::PluginView* instantiateView(QWidget* parent) override;
	FloatModel m_shape;
	FloatModel m_jitter;
};

namespace gui
{
	class HyperPipeView : public InstrumentView //InstrumentViewFixedSize
	{
		Q_OBJECT
	public:
		HyperPipeView(HyperPipe* instrument, QWidget* parent);
		virtual ~HyperPipeView();
	private:
		Knob m_shape;
		Knob m_jitter;
	};
}

} // namespace lmms
