/*
 * HyperPipe.h - declaration of all HyperPipe classes; includes; "using namespace"s
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

using namespace lmms;
using namespace lmms::gui;

namespace lmms::hyperpipe {}
using namespace lmms::hyperpipe;
namespace lmms::gui::hyperpipe {}
using namespace lmms::gui::hyperpipe;

namespace lmms::hyperpipe
{

class HPNode;

/*!
 * The HyperPipe data model.
 * An instance of this class contains the plugin part of a preset.
 */
class HPModel : QObject
{
	Q_OBJECT
public:
	HPModel(Instrument* instrument);
	struct Node {
		vector<shared_ptr<ComboBoxModel>> m_cbmodels;
		vector<shared_ptr<FloatModel>> m_fmodels;
		vector<shared_ptr<IntModel>> m_imodels;
		virtual shared_ptr<HPNode> instantiate(shared_ptr<Node> self) = 0;
		virtual string name() = 0;
	};
	struct Noise : public Node {
		Noise(Instrument* instrument);
		shared_ptr<FloatModel> m_spike;
		shared_ptr<HPNode> instantiate(shared_ptr<Node> self);
		string name();
		Instrument* m_instrument;
	};
	struct Sine : public Node {
		Sine(Instrument* instrument);
		shared_ptr<FloatModel> m_sawify;
		shared_ptr<HPNode> instantiate(shared_ptr<Node> self);
		string name();
	};
	struct Shapes : public Node {
		Shapes(Instrument* instrument);
		shared_ptr<FloatModel> m_shape;
		shared_ptr<FloatModel> m_jitter;
		shared_ptr<HPNode> instantiate(shared_ptr<Node> self);
		string name();
	};
	vector<shared_ptr<Node>> m_nodes;
};

class HPNode : QObject
{
	Q_OBJECT
public:
	HPNode();
	virtual ~HPNode();
	virtual float processFrame(float freq, float srate) = 0;
};

class HPOsc : public HPNode
{
public:
	HPOsc();
	virtual ~HPOsc();
	float processFrame(float freq, float srate);
private:
	virtual float shape(float ph) = 0;
	float m_ph = 0.0f;
};

class HPSine : public HPOsc
{
public:
	HPSine(shared_ptr<HPModel::Sine> model);
	virtual ~HPSine();
	shared_ptr<FloatModel> m_sawify;
private:
	float shape(float ph);
};

class HPShapes : public HPOsc
{
public:
	HPShapes(shared_ptr<HPModel::Shapes> model);
	virtual ~HPShapes();
	shared_ptr<FloatModel> m_shape;
	shared_ptr<FloatModel> m_jitter;
private:
	float shape(float ph);
};

class HPNoise : public HPNode
{
public:
	HPNoise(shared_ptr<HPModel::Noise> model, Instrument* instrument);
	virtual ~HPNoise();
	float processFrame(float freq, float srate);
	shared_ptr<FloatModel> m_spike;
private:
	HPSine m_osc;
};

class HyperPipe;

class HPSynth
{
public:
	HPSynth(HyperPipe* instrument, NotePlayHandle* nph, HPModel* model);
	virtual ~HPSynth();
	array<float,2> processFrame(float freq, float srate);
private:
	HyperPipe *m_instrument;
	NotePlayHandle *m_nph;
	shared_ptr<HPNode> m_lastNode;
};

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
	HPModel m_model;
};

} // namespace lmms::hyperpipe

namespace lmms::gui::hyperpipe {

class HPNodeView;

class HPView : public InstrumentView //InstrumentViewFixedSize
{
	Q_OBJECT
public:
	HPView(HyperPipe* instrument, QWidget* parent);
	virtual ~HPView();
private:
	ComboBox m_ntype;
	unique_ptr<HPNodeView> m_curNode;
};

class HPNodeView {
public:
	virtual ~HPNodeView();
	virtual void hide() = 0;
	virtual void show() = 0;
	virtual string name() = 0;
};

class HPNoiseView : public HPNodeView {
public:
	HPNoiseView(HPView* view, HyperPipe* instrument, shared_ptr<HPModel::Noise> model);
	void hide();
	void show();
	string name();
private:
	Knob m_spike;
};

class HPSineView : public HPNodeView {
public:
	HPSineView(HPView* view, HyperPipe* instrument, shared_ptr<HPModel::Sine> model);
	void hide();
	void show();
	string name();
private:
	Knob m_sawify;
};

class HPShapesView : public HPNodeView {
public:
	HPShapesView(HPView* view, HyperPipe* instrument, shared_ptr<HPModel::Shapes> model);
	void hide();
	void show();
	string name();
private:
	Knob m_shape;
	Knob m_jitter;
};

} // namespace lmms::gui::hyperpipe