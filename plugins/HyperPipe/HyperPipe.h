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

#include <stdexcept>
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
		virtual ~Node() = default;
		/*! Calls the synth node constructor that corresponds to the derived model struct. */
		virtual unique_ptr<HPNode> instantiate(shared_ptr<Node> self) = 0;
		virtual string name() = 0;
	};
	struct Noise : public Node {
		Noise(Instrument* instrument);
		shared_ptr<FloatModel> m_spike;
		unique_ptr<HPNode> instantiate(shared_ptr<Node> self);
		string name();
	};
	struct Sine : public Node {
		Sine(Instrument* instrument);
		shared_ptr<FloatModel> m_sawify;
		unique_ptr<HPNode> instantiate(shared_ptr<Node> self);
		string name();
	};
	struct Shapes : public Node {
		Shapes(Instrument* instrument);
		shared_ptr<FloatModel> m_shape;
		shared_ptr<FloatModel> m_jitter;
		unique_ptr<HPNode> instantiate(shared_ptr<Node> self);
		string name();
	};
	vector<shared_ptr<Node>> m_nodes;
};

class HPNode : QObject
{
	Q_OBJECT
public:
	virtual ~HPNode() = default;
	virtual float processFrame(float freq, float srate) = 0;
};

class HPOsc : public HPNode
{
public:
	virtual ~HPOsc() = default;
	float processFrame(float freq, float srate);
private:
	virtual float shape(float ph) = 0;
	float m_ph = 0.0f;
};

class HPSine : public HPOsc
{
public:
	HPSine(shared_ptr<HPModel::Sine> model);
	shared_ptr<FloatModel> m_sawify = nullptr;
	float m_sawify_fb = 0.0f; //fallback if no model
private:
	float shape(float ph);
};

class HPShapes : public HPOsc
{
public:
	HPShapes(shared_ptr<HPModel::Shapes> model);
	shared_ptr<FloatModel> m_shape = nullptr;
	float m_shape_fb = 0.0f;
	shared_ptr<FloatModel> m_jitter = nullptr;
	float m_jitter_fb = 0.0f;
private:
	float shape(float ph);
};

class HPNoise : public HPNode
{
public:
	HPNoise(shared_ptr<HPModel::Noise> model);
	float processFrame(float freq, float srate);
	shared_ptr<FloatModel> m_spike = nullptr;
	float m_spike_fb = 4.0f;
private:
	HPSine m_osc;
};

class HPInstrument;

class HPSynth
{
public:
	HPSynth(HPInstrument* instrument, NotePlayHandle* nph, HPModel* model);
	array<float,2> processFrame(float freq, float srate);
private:
	HPInstrument *m_instrument;
	NotePlayHandle *m_nph;
	unique_ptr<HPNode> m_lastNode;
};

class HPInstrument : public Instrument
{
	Q_OBJECT
public:
	HPInstrument(InstrumentTrack* track);
	void chNodeType(string nodeType, size_t model_i);
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

class HPView;

class HPNodeView {
public:
	virtual ~HPNodeView() = default;
	virtual string name() = 0;
	void hide();
	void moveRel(int x, int y);
	void show();
	vector<QWidget*> m_widgets;
};

class HPNoiseView : public HPNodeView {
public:
	HPNoiseView(HPView* view, HPInstrument* instrument);
	string name();
	void setModel(shared_ptr<HPModel::Noise> model);
private:
	Knob m_spike;
};

class HPSineView : public HPNodeView {
public:
	HPSineView(HPView* view, HPInstrument* instrument);
	string name();
	void setModel(shared_ptr<HPModel::Sine> model);
private:
	Knob m_sawify;
};

class HPShapesView : public HPNodeView {
public:
	HPShapesView(HPView* view, HPInstrument* instrument);
	string name();
	void setModel(shared_ptr<HPModel::Shapes> model);
private:
	Knob m_shape;
	Knob m_jitter;
};

class HPView : public InstrumentView //InstrumentViewFixedSize
{
	Q_OBJECT
public:
	HPView(HPInstrument* instrument, QWidget* parent);
private:
	HPNodeView *m_curNode = nullptr;
	HPNoiseView m_noise;
	HPShapesView m_shapes;
	HPSineView m_sine;
	HPInstrument *m_instrument;
	size_t m_model_i = 0;
	ComboBox m_nodeType;
	ComboBoxModel m_nodeTypeModel;
private slots:
	void chNodeType();
};

} // namespace lmms::gui::hyperpipe