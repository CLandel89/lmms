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
#include "LedCheckBox.h"
#include "lmms_math.h"
#include "NotePlayHandle.h"
#include "plugin_export.h"
#include "Song.h"

#include <map>
#include <random>
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
		/**
			Will recurse into the "instantiate" methods of its "previous" node and
			"arguments"; may do so multiple times (e.g., "organify").
		*/
		virtual unique_ptr<HPNode> instantiate(HPModel* model, int model_i) = 0;
		IntModel m_pipe;
		IntModel m_customPrev;
		//! "Argument" pipes which mix with or modulate the "current" pipe.
		vector<unique_ptr<IntModel>> m_arguments;
		virtual string name() = 0;
		virtual void load(int model_i, const QDomElement& elem) = 0;
		virtual void save(int model_i, QDomDocument& doc, QDomElement& elem) = 0;
		int prevPipe();
		virtual bool usesPrev() = 0;
	};
	//! Instantiates the "previous" node; if there is none, returns nullptr.
	unique_ptr<HPNode> instantiatePrev(int i);
	//! Instantiates the "argument" nodes; may be smaller than node->m_arguments, but never contains nullptr.
	vector<unique_ptr<HPNode>> instantiateArguments(int i);
	static unique_ptr<IntModel> newArgument(Instrument* instrument, int i);
	vector<shared_ptr<Node>> m_nodes; // shared ptr: notes being played can still access potentially removed data models
};

/**
	Base class for any HP synth and effect node.
*/
class HPNode
{
public:
	struct Params {
		float freq, freqMod, srate, ph;
	};
	virtual ~HPNode() = default;
	virtual float processFrame(Params p) = 0;
	virtual void resetState();
};

struct HPOscModel : public HPModel::Node {
	HPOscModel(Instrument* instrument);
	virtual ~HPOscModel() = default;
	virtual void load(int model_i, const QDomElement& elem);
	virtual void loadImpl(int model_i, const QDomElement& elem) = 0;
	virtual void save(int model_i, QDomDocument& doc, QDomElement& elem);
	virtual void saveImpl(int model_i, QDomDocument& doc, QDomElement& elem) = 0;
	virtual bool usesPrev();
	FloatModel m_ph;
};

class HPOsc : public HPNode
{
public:
	HPOsc(HPModel* model, int model_i, shared_ptr<HPOscModel> nmodel);
	virtual ~HPOsc() = default;
	float processFrame(Params p);
	void resetState() override;
private:
	shared_ptr<HPOscModel> m_nmodel;
	unique_ptr<HPNode> m_prev;
	vector<unique_ptr<HPNode>> m_arguments;
	virtual float shape(float ph) = 0;
};

class HPInstrument;

/**
	Every note played by HP is represented by an instance of this class.
	Root of HPNode instance creation.
*/
class HPSynth
{
public:
	HPSynth(HPInstrument* instrument, NotePlayHandle* nph, HPModel* model);
	array<float,2> processFrame(float freq, float srate);
private:
	float m_ph = 0.0f;
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
	~HPInstrument() override = default;
	void chNodeType(string nodeType, int model_i);
	void playNote(NotePlayHandle* nph, SampleFrame* working_buffer) override;
	void deleteNotePluginData(NotePlayHandle* nph) override;
	void saveSettings(QDomDocument& doc, QDomElement& elem) override;
	void loadSettings(const QDomElement& elem) override;
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
	virtual void setModel(weak_ptr<HPModel::Node> nmodel) = 0;
	vector<QWidget*> m_widgets; //!< for "show/hide" and "move"
};

class HPVArguments : QObject {
	Q_OBJECT
public:
	HPVArguments(HPView* view, HPInstrument* instrument);
	~HPVArguments();
	void setModel(weak_ptr<HPModel::Node> nmodel);
private:
	void update();
	HPInstrument *m_instrument;
	HPView *m_view;
	weak_ptr<HPModel::Node> m_nmodel;
	int m_pos = 0;
	vector<LcdSpinBox*> m_pipes;
	PixmapButton *m_left;
	PixmapButton *m_right;
	PixmapButton *m_add;
	PixmapButton *m_delete;
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
	~HPView() override;
	inline int model_i() { return m_model_i; }
	void setModel_i(int i);
	void updateWidgets();
private:
	void updateNodeView();
	map<string, unique_ptr<HPNodeView>> m_nodeViews;
	HPNodeView *m_curNode = nullptr;
	HPInstrument *m_instrument;
	int m_model_i = 0;
	Knob* m_ph;
	ComboBox *m_nodeType;
	ComboBoxModel m_nodeTypeModel;
	LcdSpinBox *m_pipe;
	LcdSpinBox *m_customPrev;
	PixmapButton *m_prev;
	PixmapButton *m_next;
	PixmapButton *m_moveUp;
	PixmapButton *m_prepend;
	PixmapButton *m_delete;
	PixmapButton *m_append;
	PixmapButton *m_moveDown;
	HPVArguments m_arguments;
	bool m_destructing = false;
	class MapWidget;
	MapWidget *m_map;
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

class HPView::MapWidget : public QWidget {
	Q_OBJECT
public:
	MapWidget(HPView* parent);
	~MapWidget() = default;
	HPView *m_parent;
	HPModel *m_model = nullptr;
protected:
	void mousePressEvent(QMouseEvent* ev) override;
	void paintEvent(QPaintEvent* ev) override;
	void wheelEvent(QWheelEvent* ev) override;
private:
	map<string, QColor> m_colors;
	QTimer *m_timer;
private slots:
	void sl_update();
};

} // namespace lmms::gui::hyperpipe

namespace lmms::hyperpipe {

/// Base class for any HyperPipe node type.
/**
	This ensures that most of the code for a specific node type can be gathered in one place.
	Each supported node type is represented by an instance (in every instance of HPInstrument).
	The "definition" then provides one view object, and any number of node model instances, which (each):
	 - can save/load its individual parameters
	 - and in turn instantiate any number of synths.
*/
class HPDefinitionBase {
public:
	HPDefinitionBase(HPInstrument* instrument);
	virtual ~HPDefinitionBase() = default;
	bool forbidsArguments();
	virtual string name() = 0;
	virtual unique_ptr<HPModel::Node> newNode() = 0;
	virtual unique_ptr<HPNodeView> instantiateView(HPView* hpview) = 0;
	static const string DEFAULT_TYPE;
protected:
	HPInstrument *m_instrument;
	bool m_forbidsArguments = false;
};

template<class M>
class HPDefinition : public HPDefinitionBase {
public:
	HPDefinition(HPInstrument* instrument);
	~HPDefinition();
	string name();
	unique_ptr<HPModel::Node> newNode() {
		return newNodeImpl();
	}
	unique_ptr<M> newNodeImpl();
	unique_ptr<HPNodeView> instantiateView(HPView* hpview);
private:
	//! Any additional members can be defined in Impl, if necessary.
	struct Impl;
	shared_ptr<Impl> m_impl = nullptr;
};

// The types for M are:
struct HPAmModel;
struct HPAmpModel;
struct HPCrushModel;
struct HPEnvModel;
struct HPFilterModel;
struct HPFmModel;
struct HPLevelerModel;
struct HPLfoModel;
struct HPMixModel;
struct HPNoiseModel;
struct HPNoiseChipModel;
struct HPOrganifyModel;
struct HPOverdriveModel;
struct HPReverbSCModel;
struct HPShapesModel;
struct HPSineModel;
struct HPSquareModel;
struct HPTransitionModel;
struct HPTuneModel;

}

// any ad-hoc utilities
namespace lmms::hyperpipe {

/**  A counter-based pseudo-random number generator for noise,
  *  built on std::minstd_rand.  */
class HPcbrng {
	uint16_t m_seed;
	uint32_t m_last_c = 123;
	uint16_t m_last_out;
public:
	HPcbrng(uint16_t seed) :
		m_seed(seed),
		m_last_out((*this)(0))
	{}
	uint16_t operator()(uint32_t c) {
		if (c == m_last_c) {
			return m_last_out;
		}
		// calculate result
		minstd_rand rng(c + m_seed * 0x10000);
		uint32_t result = 0;
		for (int _ = 0; _ < 8; _++) {
			result ^= rng();
		}
		result = result ^ (result >> 0x10);
		// update state and return result
		m_last_c = c;
		m_last_out = result;
		return result;
	}
};

inline float hpposmodf(float a, float b = 1) {
	return fmod(fmod(a, b) + b, b);
}
inline float hpposmodi(int a, int b) {
	return (a % b + b) % b;
}

/**
	Smooth step, cosine-based.
	Smoothes out rough changes near 0.0f and 1.0f.
*/
inline float hpsstep(float a) {
	// cos: 1.0...-1.0 (...1.0)
	// -cos: -1.0...1.0
	// -cos + 1.0: 0.0...2.0
	return (-cosf(a * F_PI) + 1.0f) / 2.0f;
}

//! A simple hasher for mapping names to numbers.
/** Only 16-bit because the *.xpf files will contain exponential notation otherwise. */
inline int16_t hphash(string name) {
	// basically https://dev.to/muiz6/string-hashing-in-c-1np3
	static const int PRIME_CONST = 31;
	int16_t result = 0;
	int16_t primePower = 1;
	for (char c : name) {
		result += c * primePower;
		primePower *= PRIME_CONST;
	}
	return result;
}

} // namespace lmms::hyperpipe