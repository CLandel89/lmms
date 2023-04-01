/*
	leveler.cpp - implementation of the "leveler" node type

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

#include "../HyperPipe.h"

namespace lmms::hyperpipe
{

const string LEVELER_NAME = "leveler";

inline unique_ptr<HPNode> instantiateLeveler(HPModel* model, int model_i);

struct HPLevelerModel : public HPModel::Node {
	HPLevelerModel(Instrument* instrument) :
			Node(instrument),
			m_inside(0.0f, -60.0f, 60.0f, 0.1f, instrument, QString("lvl dB inside")),
			m_outside(-30.0f, -60.0f, 60.0f, 0.1f, instrument, QString("lvl dB outside")),
			m_radius(19.0f, 0.5f, 127.0f, 0.1f, instrument, QString("lvl radius")),
			m_center(0.0f, -69.0f, 58.0f, 0.1f, instrument, QString("lvl center")),
			m_appDetune(true, instrument, QString("lvl re-apply on detune"))
	{}
	FloatModel m_inside;
	FloatModel m_outside;
	FloatModel m_radius;
	FloatModel m_center;
	BoolModel m_appDetune;
	unique_ptr<HPNode> instantiate(HPModel* model, int model_i) {
		return instantiateLeveler(model, model_i);
	}
	string name() { return LEVELER_NAME; }
	void load(int model_i, const QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_inside.loadSettings(elem, is + "_inside");
		m_outside.loadSettings(elem, is + "_outside");
		m_radius.loadSettings(elem, is + "_radius");
		m_center.loadSettings(elem, is + "_center");
		m_appDetune.loadSettings(elem, is + "_appDetune");
	}
	void save(int model_i, QDomDocument& doc, QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_inside.saveSettings(doc, elem, is + "_inside");
		m_outside.saveSettings(doc, elem, is + "_outside");
		m_radius.saveSettings(doc, elem, is + "_radius");
		m_center.saveSettings(doc, elem, is + "_center");
		m_appDetune.saveSettings(doc, elem, is + "_appDetune");
	}
	bool usesPrev() { return true; }
};

class HPLeveler : public HPNode
{
public:
	HPLeveler(HPModel* model, int model_i, shared_ptr<HPLevelerModel> nmodel) :
			m_nmodel(nmodel),
			m_prev(model->instantiatePrev(model_i))
	{}
private:
	float amp(float freq) {
		float note = 12 * log2f(freq / 440.0f);
		float db;
		float center = m_nmodel->m_center.value();
		float dist = abs(note - center);
		dist /= m_nmodel->m_radius.value();
		if (dist > 1) {
			db = m_nmodel->m_outside.value();
		}
		else {
			float interp = hpsstep(dist);
			float a = m_nmodel->m_inside.value();
			float b = m_nmodel->m_outside.value();
			db = (1 - interp) * a + interp * b;
		}
		return powf(10.0f, db / 20);
	}
	float processFrame(Params p) {
		if (m_prev == nullptr) {
			return 0.0f;
		}
		float a;
		if (m_nmodel->m_appDetune.value()) {
			// re-apply on detune
			a = amp(p.freq);
			m_ampValid = false;
		}
		else {
			// apply once and keep as m_amp
			if (m_ampValid) {
				a = m_amp;
			}
			else {
				a = m_amp = amp(p.freq);
				m_ampValid = true;
			}
		}
		return a * m_prev->processFrame(p);
	}
	void resetState() override {
		HPNode::resetState();
		if (m_prev != nullptr) {
			m_prev->resetState();
		}
		m_ampValid = false;
	}
	shared_ptr<HPLevelerModel> m_nmodel;
	unique_ptr<HPNode> m_prev = nullptr;
	float m_amp = 0;
	bool m_ampValid = false;
};

inline unique_ptr<HPNode> instantiateLeveler(HPModel* model, int model_i) {
	return make_unique<HPLeveler>(
		model,
		model_i,
		static_pointer_cast<HPLevelerModel>(model->m_nodes[model_i])
	);
}

class HPLevelerView : public HPNodeView {
public:
	HPLevelerView(HPView* view) :
			m_inside(new Knob(view, "dB (inside radius)")),
			m_outside(new Knob(view, "dB (outside radius)")),
			m_radius(new Knob(view, "radius (halftones)")),
			m_center(new Knob(view, "center (halftones from A4)")),
			m_appDetune(new LedCheckBox(view, "re-apply on detune"))
	{
		m_widgets.emplace_back(m_inside);
		m_outside->move(30, 0);
		m_widgets.emplace_back(m_outside);
		m_radius->move(60, 0);
		m_widgets.emplace_back(m_radius);
		m_center->move(90, 0);
		m_widgets.emplace_back(m_center);
		m_appDetune->move(120, 0);
		m_widgets.emplace_back(m_appDetune);
	}
	void setModel(weak_ptr<HPModel::Node> nmodel) {
		auto modelCast = static_cast<HPLevelerModel*>(nmodel.lock().get());
		m_inside->setModel(&modelCast->m_inside);
		m_outside->setModel(&modelCast->m_outside);
		m_radius->setModel(&modelCast->m_radius);
		m_center->setModel(&modelCast->m_center);
		m_appDetune->setModel(&modelCast->m_appDetune);
	}
private:
	Knob *m_inside;
	Knob *m_outside;
	Knob *m_radius;
	Knob *m_center;
	LedCheckBox *m_appDetune;
};

using Definition = HPDefinition<HPLevelerModel>;

template<> Definition::HPDefinition(HPInstrument* instrument) :
		HPDefinitionBase(instrument)
{
		m_forbidsArguments = true;
}

template<> Definition::~HPDefinition() = default;

template<> string Definition::name() { return LEVELER_NAME; }

template<> unique_ptr<HPLevelerModel> Definition::newNodeImpl() {
	return make_unique<HPLevelerModel>(m_instrument);
}

template<> unique_ptr<HPNodeView> Definition::instantiateView(HPView* hpview) {
	return make_unique<HPLevelerView>(hpview);
}

} // namespace lmms::hyperpipe
