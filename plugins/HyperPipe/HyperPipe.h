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

#include "Instrument.h"
#include "InstrumentView.h"
#include "AutomatableModel.h"

#include <vector>

namespace lmms
{

class NotePlayHandle;
class SampleBuffer;

namespace gui
{
class automatableButtonGroup;
class Knob;
class PixmapButton;
class TripleOscillatorView;
} // namespace gui

class HyperPipeNode : public Model
{
public:
    HyperPipeNode(Model* _parent);
    virtual ~HyperPipeNode();
};

class HyperPipeOsc : HyperPipeNode
{
public:
    HyperPipeOsc(Model* _parent);
    virtual ~HyperPipeOsc();
};

class HyperPipe;

class HyperPipeSynth
{
    MM_OPERATORS
public:
    HyperPipeSynth(HyperPipe* _i, NotePlayHandle* _nph);
    virtual ~HyperPipeSynth();
	std::array<float,2> processFrame(float freq, float srate);
    float m_ph = 0;
private:
    HyperPipe *m_parent;
    NotePlayHandle *m_nph;
};

namespace gui {
    class HyperPipeView;
}

class HyperPipe : public Instrument
{
	Q_OBJECT
public:
	HyperPipe(InstrumentTrack* _track);
	virtual ~HyperPipe();
	void playNote(NotePlayHandle* _n, sampleFrame* _working_buffer) override;
	void deleteNotePluginData(NotePlayHandle* _n) override;
	void saveSettings(QDomDocument& _doc, QDomElement& _parent) override;
	void loadSettings(const QDomElement& _this) override;
	QString nodeName() const override;
	gui::PluginView* instantiateView(QWidget* _parent) override;
private:
	std::vector<HyperPipeNode> m_nodes;
	friend class gui::HyperPipeView;
};


namespace gui
{

class HyperPipeView : public InstrumentView //InstrumentViewFixedSize
{
	Q_OBJECT
public:
	HyperPipeView(Instrument *_instrument, QWidget *_parent);
	virtual ~HyperPipeView();
private:
	void modelChanged() override;
};


} // namespace gui

} // namespace lmms