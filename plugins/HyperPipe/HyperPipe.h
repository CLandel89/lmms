/*
	HyperPipe.h - declaration of common HyperPipe classes; includes; "using namespace"s

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

#include "AudioEngine.h"
#include "PixmapButton.h"
#include "ComboBox.h"
#include "Engine.h"
#include "Instrument.h"
#include "InstrumentView.h"
#include "InstrumentTrack.h"
#include "Knob.h"
#include "LcdSpinBox.h"
#include "lmms_math.h"
#include "NotePlayHandle.h"
#include "plugin_export.h"

#include <map>
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

const string HP_DEFAULT_NODE_TYPE;

class HPInstrument;
class HPNode;

/**
	The HyperPipe data model.
	An instance of this class contains the plugin part of a preset.
*/
class HPModel
{
public:
	HPModel(HPInstrument* instrument);
	struct Node {
		Node(Instrument* instrument);
		virtual ~Node() = default;
		//! Calls the synth node constructor that corresponds to the derived model struct.
		virtual unique_ptr<HPNode> instantiate(shared_ptr<Node> self) = 0;
		shared_ptr<IntModel> m_pipe;
		//! "Argument" pipes which mix with or modulate the "current" pipe.
		vector<shared_ptr<IntModel>> m_arguments;
		virtual string name() = 0;
		virtual void load(string params) = 0;
		virtual string save() = 0;
	};
	vector<shared_ptr<Node>> m_nodes;
	void prepend(shared_ptr<Node> node, int model_i);
	void append(shared_ptr<Node> node, int model_i);
	void remove(int model_i);
	int size();
};

/**
	Base class for any HP synth and effect node.
*/
class HPNode
{
public:
	virtual ~HPNode() = default;
	virtual float processFrame(float freq, float srate) = 0;
	//! Previous node with same pipe № which HPSynth puts here.
	unique_ptr<HPNode> m_prev = nullptr;
	//! "Argument" nodes which HPSynth puts here.
	vector<unique_ptr<HPNode>> m_arguments;
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

class HPInstrument;

/**
	Every note played by HP is represented by an instance of this class.
	Creates and deletes HPNode instances.
*/
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

class HPDefinitionBase;

class HPInstrument : public Instrument
{
	Q_OBJECT
public:
	HPInstrument(InstrumentTrack* track);
	void chNodeType(string nodeType, int model_i);
	void playNote(NotePlayHandle* nph, sampleFrame* working_buffer) override;
	void deleteNotePluginData(NotePlayHandle* nph) override;
	void saveSettings(QDomDocument& doc, QDomElement& parent) override;
	void loadSettings(const QDomElement& preset) override;
	QString nodeName() const override;
	gui::PluginView* instantiateView(QWidget* parent) override;
	map<string, unique_ptr<HPDefinitionBase>> m_definitions;
	HPModel m_model;
};

} // namespace lmms::hyperpipe

namespace lmms::gui::hyperpipe {

class HPView;

class HPNodeView {
public:
	virtual ~HPNodeView() = default;
	void hide();
	void moveRel(int x, int y);
	void show();
	virtual void setModel(shared_ptr<HPModel::Node> model) = 0;
	vector<QWidget*> m_widgets;
};

class HPVArguments : QObject {
	Q_OBJECT
public:
	HPVArguments(HPView* view, HPInstrument* instrument);
	~HPVArguments();
	void setModel(shared_ptr<HPModel::Node> model);
private:
	void update();
	HPInstrument *m_instrument;
	shared_ptr<HPModel::Node> m_model = nullptr;
	int m_pos = 0;
	vector<unique_ptr<LcdSpinBox>> m_pipes;
	PixmapButton m_left;
	PixmapButton m_right;
	PixmapButton m_add;
	PixmapButton m_delete;
	bool m_destructing = false;
private slots:
	void sl_left();
	void sl_right();
	void sl_add();
	void sl_delete();
};

class HPView : public InstrumentView //InstrumentViewFixedSize
{
	Q_OBJECT
public:
	HPView(HPInstrument* instrument, QWidget* parent);
	~HPView();
private:
	void updateNodeView();
	map<string, unique_ptr<HPNodeView>> m_nodeViews;
	HPNodeView *m_curNode = nullptr;
	HPInstrument *m_instrument;
	int m_model_i = 0;
	ComboBox m_nodeType;
	ComboBoxModel m_nodeTypeModel;
	LcdSpinBox m_pipe;
	PixmapButton m_prev;
	PixmapButton m_next;
	PixmapButton m_moveUp;
	PixmapButton m_prepend;
	PixmapButton m_delete;
	PixmapButton m_append;
	PixmapButton m_moveDown;
	HPVArguments m_arguments;
	bool m_destructing = false;

private slots:
	void sl_chNodeType();
	void sl_prev();
	void sl_next();
	void sl_moveUp();
	void sl_prepend();
	void sl_delete();
	void sl_append();
	void sl_moveDown();
};

} // namespace lmms::gui::hyperpipe

namespace lmms::hyperpipe {

/// Base class for any HyperPipe node type.
/**
	This ensures that most of the code for a specific node type can be gathered in one place.
	Each supported node type is represented by an instance (in every instance of HPInstrument).
	The "definition" then provides one view object, and any number of node model instances, which (each):
	 - can save/load its (individual) parameters into/from a std::string (no newlines allowed!)
	 - and in turn instantiate any number of synths.
*/
class HPDefinitionBase {
public:
	HPDefinitionBase(HPInstrument* instrument);
	virtual ~HPDefinitionBase() = default;
	virtual string name() = 0;
	virtual shared_ptr<HPModel::Node> newNode() = 0;
	virtual unique_ptr<HPNodeView> instantiateView(HPView* hpview) = 0;
	static const string DEFAULT_TYPE;
protected:
	HPInstrument *m_instrument;
};

template<class M>
class HPDefinition : public HPDefinitionBase {
public:
	HPDefinition(HPInstrument* instrument);
	~HPDefinition();
	string name();
	shared_ptr<HPModel::Node> newNode() {
		return newNodeImpl();
	}
	shared_ptr<M> newNodeImpl();
	unique_ptr<HPNodeView> instantiateView(HPView* hpview);
private:
	//! Any additional members can be defined in Impl, if necessary.
	struct Impl;
	shared_ptr<Impl> m_impl = nullptr;
};

// The types for M are:
struct HPNoiseModel;
struct HPShapesModel;
struct HPSineModel;

}

// any ad-hoc utilities
namespace lmms::hyperpipe {
/**
	Smooth step, cosine-based.
	Smoothes out rough changes near 0.0f and 1.0f.
*/
inline float sstep(float a) {
	// cos: 1.0...-1.0 (...1.0)
	// -cos: -1.0...1.0
	// -cos + 1.0: 0.0...2.0
	return (-cosf(a * F_PI) + 1.0f) / 2.0f;
}
}