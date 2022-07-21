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

#include <vector>

using namespace std;

namespace lmms
{

// the syntheziser nodes and their abstract class
class HyperPipeNode;
class HyperPipeNoise;
class HyperPipeSine;
class HyperPipeShapes;

/*!
 * The HyperPipe data model.
 * An instance of this class contains the plugin part of a preset.
 */
class HyperPipeModel : QObject
{
	Q_OBJECT
public:
	HyperPipeModel(Instrument* instrument);
	struct Node {
		vector<shared_ptr<ComboBoxModel>> m_cbmodels;
		vector<shared_ptr<FloatModel>> m_fmodels;
		vector<shared_ptr<IntModel>> m_imodels;
		virtual shared_ptr<HyperPipeNode> instantiate(shared_ptr<Node> self) = 0;
		virtual string name() = 0;
	};
	struct Noise : public Node {
		Noise(Instrument* instrument);
		shared_ptr<FloatModel> m_spike;
		shared_ptr<HyperPipeNode> instantiate(shared_ptr<Node> self, Instrument* instrument);
		string name();
	};
	struct Sine : public Node {
		Sine(Instrument* instrument);
		shared_ptr<FloatModel> m_sawify;
		shared_ptr<HyperPipeNode> instantiate(shared_ptr<Node> self);
		string name();
	};
	struct Shapes : public Node {
		Shapes(Instrument* instrument);
		shared_ptr<FloatModel> m_shape;
		shared_ptr<FloatModel> m_jitter;
		shared_ptr<HyperPipeNode> instantiate(shared_ptr<Node> self);
		string name();
	};
	vector<shared_ptr<Node>> m_nodes;
};

class HyperPipeNode : QObject
{
	Q_OBJECT
public:
	HyperPipeNode();
	virtual ~HyperPipeNode();
	virtual float processFrame(float freq, float srate) = 0;
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
	HyperPipeSine(shared_ptr<HyperPipeModel::Sine> model);
	virtual ~HyperPipeSine();
	shared_ptr<FloatModel> m_sawify;
private:
	float shape(float ph);
};

class HyperPipeShapes : public HyperPipeOsc
{
public:
	HyperPipeShapes(shared_ptr<HyperPipeModel::Shapes> model);
	virtual ~HyperPipeShapes();
	shared_ptr<FloatModel> m_shape;
	shared_ptr<FloatModel> m_jitter;
private:
	float shape(float ph);
};

class HyperPipeNoise : public HyperPipeNode
{
public:
	HyperPipeNoise(shared_ptr<HyperPipeModel::Noise> model, Instrument* instrument);
	virtual ~HyperPipeNoise();
	float processFrame(float freq, float srate);
	shared_ptr<FloatModel> m_spike;
private:
	HyperPipeSine m_osc;
};

class HyperPipe;

class HyperPipeSynth
{
	MM_OPERATORS
public:
	HyperPipeSynth(HyperPipe* instrument, NotePlayHandle* nph, HyperPipeModel* model);
	virtual ~HyperPipeSynth();
	array<float,2> processFrame(float freq, float srate);
private:
	HyperPipe *m_instrument;
	NotePlayHandle *m_nph;
	shared_ptr<HyperPipeNode> m_lastNode;
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
	unique_ptr<HyperPipeModel> m_model;
};

namespace gui
{
	class HyperPipeNodeView;

	class HyperPipeView : public InstrumentView //InstrumentViewFixedSize
	{
		Q_OBJECT
	public:
		HyperPipeView(HyperPipe* instrument, QWidget* parent);
		virtual ~HyperPipeView();
	private:
		ComboBox m_ntype;
		unique_ptr<HyperPipeNodeView> m_curNode;
	};

	class HyperPipeNodeView {
	public:
		virtual ~HyperPipeNodeView();
		virtual void hide() = 0;
		virtual void show() = 0;
		virtual string name() = 0;
	};

	class HyperPipeNoiseView : public HyperPipeNodeView {
	public:
		HyperPipeNoiseView(HyperPipeView* view, HyperPipe* instrument, shared_ptr<HyperPipeModel::Noise> model);
		void hide();
		void show();
		string name();
	private:
		Knob m_spike;
	};

	class HyperPipeSineView : public HyperPipeNodeView {
	public:
		HyperPipeSineView(HyperPipeView* view, HyperPipe* instrument, shared_ptr<HyperPipeModel::Sine> model);
		void hide();
		void show();
		string name();
	private:
		Knob m_sawify;
	};

	class HyperPipeShapesView : public HyperPipeNodeView {
	public:
		HyperPipeShapesView(HyperPipeView* view, HyperPipe* instrument, shared_ptr<HyperPipeModel::Shapes> model);
		void hide();
		void show();
		string name();
	private:
		Knob m_shape;
		Knob m_jitter;
	};
}

} // namespace lmms
