/*
	env.cpp - implementation of the "env" node type

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

const string ENV_NAME = "env";

inline unique_ptr<HPNode> instantiateEnv(HPModel* model, int model_i);

struct HPEnvModel : public HPModel::Node {
	HPEnvModel(Instrument* instrument) :
			Node(instrument),
			m_amt(1.0f, -1.0f, 1.0f, 0.01f, instrument, QString("Env amount")),
			m_exp(2.0f, 0.01f, 10.0f, 0.01f, instrument, QString("exp")),
			m_stretch(0.0f, -10.0f, 10.0f, 0.01f, instrument, QString("stretch")),
			m_del(0.0f, 0.0f, 5.0f, 0.001f, instrument, QString("delay")),
			m_att(0.03f, 0.0f, 5.0f, 0.001f, instrument, QString("attack")),
			m_hold(0.00f, 0.0f, 5.0f, 0.001f, instrument, QString("hold")),
			m_dec(1.0f, 0.0f, 5.0f, 0.001f, instrument, QString("decay")),
			m_sus(0.5f, 0.0f, 1.0f, 0.01f, instrument, QString("sustain")),
			m_smooth(false, instrument, QString("smooth"))
	{}
	FloatModel m_amt;
	FloatModel m_exp;
	FloatModel m_stretch;
	FloatModel m_del;
	FloatModel m_att;
	FloatModel m_hold;
	FloatModel m_dec;
	FloatModel m_sus;
	BoolModel m_smooth;
	unique_ptr<HPNode> instantiate(HPModel* model, int model_i) {
		return instantiateEnv(model, model_i);
	}
	string name() { return ENV_NAME; }
	void load(int model_i, const QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_amt.loadSettings(elem, is + "_amt");
		m_exp.loadSettings(elem, is + "_exp");
		m_stretch.loadSettings(elem, is + "_stretch");
		m_del.loadSettings(elem, is + "_del");
		m_att.loadSettings(elem, is + "_att");
		m_hold.loadSettings(elem, is + "_hold");
		m_dec.loadSettings(elem, is + "_dec");
		m_sus.loadSettings(elem, is + "_sus");
		m_smooth.loadSettings(elem, is + "_smooth");
	}
	void save(int model_i, QDomDocument& doc, QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_amt.saveSettings(doc, elem, is + "_amt");
		m_exp.saveSettings(doc, elem, is + "_exp");
		m_stretch.saveSettings(doc, elem, is + "_stretch");
		m_del.saveSettings(doc, elem, is + "_del");
		m_att.saveSettings(doc, elem, is + "_att");
		m_hold.saveSettings(doc, elem, is + "_hold");
		m_dec.saveSettings(doc, elem, is + "_dec");
		m_sus.saveSettings(doc, elem, is + "_sus");
		m_smooth.saveSettings(doc, elem, is + "_smooth");
	}
	bool usesPrev() { return true; }
};

class HPEnv : public HPNode
{
public:
	HPEnv(HPModel* model, int model_i, shared_ptr<HPEnvModel> nmodel) :
			m_nmodel(nmodel),
			m_prev(model->instantiatePrev(model_i))
	{}
private:
	float transform(float amp) {
		if (m_nmodel->m_smooth.value()) {
			amp = hpsstep(amp);
		}
		return powf(amp, m_nmodel->m_exp.value());
	}
	float processFrame(Params p)
	{
		if (m_prev == nullptr) {
			return 0;
		}
		float stretchL = powf(440.0f / p.freq, m_nmodel->m_stretch.value());
		float del = stretchL * m_nmodel->m_del.value();
		float att = stretchL * m_nmodel->m_att.value();
		float hold = stretchL * m_nmodel->m_hold.value();
		float dec = stretchL * m_nmodel->m_dec.value();
		float sus = m_nmodel->m_sus.value();
		float amp;
		if (m_state < del) {
			amp = 0;
		}
		else if (m_state < del + att) {
			amp = (m_state - del) / att; //0.0...1.0
			amp = transform(amp); //0.0...1.0
		}
		else if (m_state < del + att + hold) {
			amp = 1;
		}
		else if (m_state < del + att + hold + dec) {
			amp = 1 - (m_state - del - att - hold) / dec; //1.0...0.0
			amp = transform(amp); //1.0...0.0
			amp = sus + amp * (1 - sus); //1.0...sus
		}
		else {
			amp = sus;
		}
		float amt = m_nmodel->m_amt.value();
		if (amt >= 0) {
			amp = 1 - amt + amt * amp; //1.0-amt...1.0
		}
		else {
			amp = 1 - (-amt) * amp; //1.0...1.0-|amt|
		}
		m_state += 1.0f / p.srate;
		return amp * m_prev->processFrame(p);
	}
	void resetState() override {
		HPNode::resetState();
		if (m_prev != nullptr) {
			m_prev->resetState();
		}
		m_state = 0.0f;
	}
	shared_ptr<HPEnvModel> m_nmodel;
	unique_ptr<HPNode> m_prev;
	float m_state = 0.0f;
};

inline unique_ptr<HPNode> instantiateEnv(HPModel* model, int model_i) {
	return make_unique<HPEnv>(
		model,
		model_i,
		static_pointer_cast<HPEnvModel>(model->m_nodes[model_i])
	);
}

class HPEnvView : public HPNodeView {
public:
	HPEnvView(HPView* view) :
			m_amt(new Knob(view, "Env amount")),
			m_exp(new Knob(view, "exp")),
			m_stretch(new Knob(view, "stretch")),
			m_del(new Knob(view, "delay")),
			m_att(new Knob(view, "attack")),
			m_hold(new Knob(view, "hold")),
			m_dec(new Knob(view, "decay")),
			m_sus(new Knob(view, "sustain")),
			m_smooth(new LedCheckBox(view, "smooth"))
	{
		m_widgets.emplace_back(m_amt);
		m_exp->move(30, 0);
		m_widgets.emplace_back(m_exp);
		m_stretch->move(60, 0);
		m_widgets.emplace_back(m_stretch);
		m_del->move(0, 30);
		m_widgets.emplace_back(m_del);
		m_att->move(30, 30);
		m_widgets.emplace_back(m_att);
		m_hold->move(60, 30);
		m_widgets.emplace_back(m_hold);
		m_dec->move(90, 30);
		m_widgets.emplace_back(m_dec);
		m_sus->move(120, 30);
		m_widgets.emplace_back(m_sus);
		m_smooth->move(150, 30);
		m_widgets.emplace_back(m_smooth);
	}
	void setModel(weak_ptr<HPModel::Node> nmodel) {
		auto modelCast = static_cast<HPEnvModel*>(nmodel.lock().get());
		m_amt->setModel(&modelCast->m_amt);
		m_exp->setModel(&modelCast->m_exp);
		m_stretch->setModel(&modelCast->m_stretch);
		m_del->setModel(&modelCast->m_del);
		m_att->setModel(&modelCast->m_att);
		m_hold->setModel(&modelCast->m_hold);
		m_dec->setModel(&modelCast->m_dec);
		m_sus->setModel(&modelCast->m_sus);
		m_smooth->setModel(&modelCast->m_smooth);
	}
private:
	Knob *m_amt;
	Knob *m_exp;
	Knob *m_stretch;
	Knob *m_del;
	Knob *m_att;
	Knob *m_hold;
	Knob *m_dec;
	Knob *m_sus;
	LedCheckBox *m_smooth;
};

using Definition = HPDefinition<HPEnvModel>;

template<> Definition::HPDefinition(HPInstrument* instrument) :
		HPDefinitionBase(instrument)
{
		m_forbidsArguments = true;
}

template<> Definition::~HPDefinition() = default;

template<> string Definition::name() { return ENV_NAME; }

template<> unique_ptr<HPEnvModel> Definition::newNodeImpl() {
	return make_unique<HPEnvModel>(m_instrument);
}

template<> unique_ptr<HPNodeView> Definition::instantiateView(HPView* hpview) {
	return make_unique<HPEnvView>(hpview);
}

} // namespace lmms::hyperpipe
