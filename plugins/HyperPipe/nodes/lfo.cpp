/*
	lfo.cpp - implementation of the "lfo" node type

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

const string LFO_NAME = "lfo";

inline unique_ptr<HPNode> instantiateLfo(HPModel* model, int model_i);

struct HPLfoModel : public HPModel::Node {
	HPLfoModel(Instrument* instrument) :
			Node(instrument),
			m_att(0.2f, 0.0f, 5.0f, 0.001f, instrument, QString("LFO attack")),
			m_amtA(5.0f, 0.0f, 30.0f, 0.1f, instrument, QString("LFO amount (-dB)")),
			m_amtT(0.0f, -24.0f, 24.0f, 0.01f, instrument, QString("LFO amount (tune)")),
			m_freq(10.0f, 0.1f, 50.0f, 0.01f, instrument, QString("LFO freq")),
			m_stretch(0.0f, -10.0f, 10.0f, 0.01f, instrument, QString("LFO stretch"))
	{}
	FloatModel m_att;
	FloatModel m_amtA;
	FloatModel m_amtT;
	FloatModel m_freq;
	FloatModel m_stretch;
	unique_ptr<HPNode> instantiate(HPModel* model, int model_i) {
		return instantiateLfo(model, model_i);
	}
	string name() { return LFO_NAME; }
	void load(int model_i, const QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_att.loadSettings(elem, is + "_att");
		m_amtA.loadSettings(elem, is + "_amtA");
		m_amtT.loadSettings(elem, is + "_amtT");
		m_freq.loadSettings(elem, is + "_freq");
		m_stretch.loadSettings(elem, is + "_stretch");
	}
	void save(int model_i, QDomDocument& doc, QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_att.saveSettings(doc, elem, is + "_att");
		m_amtA.saveSettings(doc, elem, is + "_amtA");
		m_amtT.saveSettings(doc, elem, is + "_amtT");
		m_freq.saveSettings(doc, elem, is + "_freq");
		m_stretch.saveSettings(doc, elem, is + "_stretch");
	}
	bool usesPrev() { return true; }
};

class HPLfo : public HPNode
{
public:
	HPLfo(HPModel* model, int model_i, shared_ptr<HPLfoModel> nmodel) :
			m_nmodel(nmodel),
			m_prev(model->instantiatePrev(model_i))
	{}
private:
	float processFrame(Params p) {
		if (m_prev == nullptr) {
			return 0;
		}
		if (! m_ph_valid) {
			m_ph = p.ph;
			m_ph_valid = true;
		}
		float stretch = m_nmodel->m_stretch.value();
		stretch = powf(440.0f / p.freq, stretch);
		float att = m_nmodel->m_att.value();
		att *= stretch;
		if (att == 0 || m_state > att) {
			att = 1;
		}
		else {
			att = m_state / att;
		}
		// amplify
		float a = att * m_nmodel->m_amtA.value();
		a = a + a * sinf(F_2PI * m_lfo_ph);
		a /= 2;
		a = powf(10.0f, -a / 20);
		// tune
		float t = att * m_nmodel->m_amtT.value() * sinf(F_2PI * m_lfo_ph);
		t = powf(2.0f, t / 12);
		// calculate own phase, adapt p.freqMod
		p.ph = m_ph;
		p.freqMod *= t;
		m_ph += p.freqMod / p.srate;
		m_ph = hpposmodf(m_ph);
		// change state, return result
		m_state += 1.0f / p.srate;
		float freq = m_nmodel->m_freq.value();
		m_lfo_ph += freq / p.srate / stretch;
		m_lfo_ph = hpposmodf(m_lfo_ph);
		return a * m_prev->processFrame(p);
	}
	void resetState() override {
		HPNode::resetState();
		if (m_prev != nullptr) {
			m_prev->resetState();
		}
		m_state = 0;
		m_ph_valid = false;
		m_lfo_ph = 0;
	}
	shared_ptr<HPLfoModel> m_nmodel;
	unique_ptr<HPNode> m_prev;
	float m_state = 0;
	float m_ph;
	bool m_ph_valid = false;
	float m_lfo_ph = 0;
};

inline unique_ptr<HPNode> instantiateLfo(HPModel* model, int model_i) {
	return make_unique<HPLfo>(
		model,
		model_i,
		static_pointer_cast<HPLfoModel>(model->m_nodes[model_i])
	);
}

class HPLfoView : public HPNodeView {
public:
	HPLfoView(HPView* view) :
			m_att(new Knob(view, "LFO attack")),
			m_amtA(new Knob(view, "LFO amount (-dB)")),
			m_amtT(new Knob(view, "LFO amount (tune)")),
			m_freq(new Knob(view, "LFO freq")),
			m_stretch(new Knob(view, "LFO stretch"))
	{
		m_widgets.emplace_back(m_att);
		m_amtA->move(30, 0);
		m_widgets.emplace_back(m_amtA);
		m_amtT->move(60, 0);
		m_widgets.emplace_back(m_amtT);
		m_freq->move(90, 0);
		m_widgets.emplace_back(m_freq);
		m_stretch->move(120, 0);
		m_widgets.emplace_back(m_stretch);
	}
	void setModel(weak_ptr<HPModel::Node> nmodel) {
		auto modelCast = static_cast<HPLfoModel*>(nmodel.lock().get());
		m_att->setModel(&modelCast->m_att);
		m_amtA->setModel(&modelCast->m_amtA);
		m_amtT->setModel(&modelCast->m_amtT);
		m_freq->setModel(&modelCast->m_freq);
		m_stretch->setModel(&modelCast->m_stretch);
	}
private:
	Knob *m_att;
	Knob *m_amtA;
	Knob *m_amtT;
	Knob *m_freq;
	Knob *m_stretch;
};

using Definition = HPDefinition<HPLfoModel>;

template<> Definition::HPDefinition(HPInstrument* instrument) :
		HPDefinitionBase(instrument)
{
	m_forbidsArguments = true;
}

template<> Definition::~HPDefinition() = default;

template<> string Definition::name() { return LFO_NAME; }

template<> unique_ptr<HPLfoModel> Definition::newNodeImpl() {
	return make_unique<HPLfoModel>(m_instrument);
}

template<> unique_ptr<HPNodeView> Definition::instantiateView(HPView* hpview) {
	return make_unique<HPLfoView>(hpview);
}

} // namespace lmms::hyperpipe