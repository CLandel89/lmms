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
			m_amt(make_shared<FloatModel>(1.0f, -1.0f, 1.0f, 0.01f, instrument, QString("Env amount"))),
			m_exp(make_shared<FloatModel>(2.0f, 0.01f, 10.0f, 0.01f, instrument, QString("exp"))),
			m_stretch(make_shared<FloatModel>(0.0f, -10.0f, 10.0f, 0.01f, instrument, QString("stretch"))),
			m_del(make_shared<FloatModel>(0.0f, 0.0f, 5.0f, 0.001f, instrument, QString("delay"))),
			m_att(make_shared<FloatModel>(0.03f, 0.0f, 5.0f, 0.001f, instrument, QString("attack"))),
			m_hold(make_shared<FloatModel>(0.00f, 0.0f, 5.0f, 0.001f, instrument, QString("hold"))),
			m_dec(make_shared<FloatModel>(1.0f, 0.0f, 5.0f, 0.001f, instrument, QString("decay"))),
			m_sus(make_shared<FloatModel>(0.5f, 0.0f, 1.0f, 0.01f, instrument, QString("sustain")))
	{}
	shared_ptr<FloatModel> m_amt;
	shared_ptr<FloatModel> m_exp;
	shared_ptr<FloatModel> m_stretch;
	shared_ptr<FloatModel> m_del;
	shared_ptr<FloatModel> m_att;
	shared_ptr<FloatModel> m_hold;
	shared_ptr<FloatModel> m_dec;
	shared_ptr<FloatModel> m_sus;
	unique_ptr<HPNode> instantiate(HPModel* model, int model_i) {
		return instantiateEnv(model, model_i);
	}
	string name() { return ENV_NAME; }
	void load(int model_i, const QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_amt->loadSettings(elem, is + "_amt");
		m_exp->loadSettings(elem, is + "_exp");
		m_stretch->loadSettings(elem, is + "_stretch");
		m_del->loadSettings(elem, is + "_del");
		m_att->loadSettings(elem, is + "_att");
		m_hold->loadSettings(elem, is + "_hold");
		m_dec->loadSettings(elem, is + "_dec");
		m_sus->loadSettings(elem, is + "_sus");
	}
	void save(int model_i, QDomDocument& doc, QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_amt->saveSettings(doc, elem, is + "_amt");
		m_exp->saveSettings(doc, elem, is + "_exp");
		m_stretch->saveSettings(doc, elem, is + "_stretch");
		m_del->saveSettings(doc, elem, is + "_del");
		m_att->saveSettings(doc, elem, is + "_att");
		m_hold->saveSettings(doc, elem, is + "_hold");
		m_dec->saveSettings(doc, elem, is + "_dec");
		m_sus->saveSettings(doc, elem, is + "_sus");
	}
};

class HPEnv : public HPNode
{
public:
	HPEnv(HPModel* model, int model_i, shared_ptr<HPEnvModel> nmodel) :
			m_amt(nmodel->m_amt),
			m_exp(nmodel->m_exp),
			m_stretch(nmodel->m_stretch),
			m_del(nmodel->m_del),
			m_att(nmodel->m_att),
			m_hold(nmodel->m_hold),
			m_dec(nmodel->m_dec),
			m_sus(nmodel->m_sus),
			m_prev(model->instantiatePrev(model_i))
	{}
private:
	float processFrame(float freq, float srate) {
		float stretchL = powf(440.0f / freq, m_stretch->value());
		float del = stretchL * m_del->value();
		float att = stretchL * m_att->value();
		float hold = stretchL * m_hold->value();
		float dec = stretchL * m_dec->value();
		float sus = m_sus->value();
		float a = 1;
		if (m_state < del) {
			a = 0;
		}
		else if (m_state < del + att) {
			a = (m_state - del) / att;
			a = powf(a, m_exp->value());
		}
		else if (m_state < del + att + hold) {
			//a = 1
		}
		else if (m_state < del + att + hold + dec) {
			a = 1 - (m_state - del - att - hold) / dec;
			a = powf(a, m_exp->value());
			a = sus + a * (1 - sus);
		}
		else {
			a = sus;
		}
		float amt = m_amt->value();
		if (amt >= 0) {
			a = amt * a + 1 - amt;
		}
		else {
			a = 1 - (-amt) * a;
		}
		m_state += 1.0f / srate;
		if (m_prev == nullptr) {
			return 0;
		}
		return a * m_prev->processFrame(freq, srate);
	}
	shared_ptr<FloatModel> m_amt;
	shared_ptr<FloatModel> m_exp;
	shared_ptr<FloatModel> m_stretch;
	shared_ptr<FloatModel> m_del;
	shared_ptr<FloatModel> m_att;
	shared_ptr<FloatModel> m_hold;
	shared_ptr<FloatModel> m_dec;
	shared_ptr<FloatModel> m_sus;
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
			m_amt(view, "Env amount"),
			m_exp(view, "exp"),
			m_stretch(view, "stretch"),
			m_del(view, "delay"),
			m_att(view, "attack"),
			m_hold(view, "hold"),
			m_dec(view, "decay"),
			m_sus(view, "sustain")
	{
		m_widgets.emplace_back(&m_amt);
		m_exp.move(30, 0);
		m_widgets.emplace_back(&m_exp);
		m_stretch.move(60, 0);
		m_widgets.emplace_back(&m_stretch);
		m_del.move(0, 30);
		m_widgets.emplace_back(&m_del);
		m_att.move(30, 30);
		m_widgets.emplace_back(&m_att);
		m_hold.move(60, 30);
		m_widgets.emplace_back(&m_hold);
		m_dec.move(90, 30);
		m_widgets.emplace_back(&m_dec);
		m_sus.move(120, 30);
		m_widgets.emplace_back(&m_sus);
	}
	void setModel(shared_ptr<HPModel::Node> model) {
		shared_ptr<HPEnvModel> modelCast = static_pointer_cast<HPEnvModel>(model);
		m_amt.setModel(modelCast->m_amt.get());
		m_exp.setModel(modelCast->m_exp.get());
		m_stretch.setModel(modelCast->m_stretch.get());
		m_del.setModel(modelCast->m_del.get());
		m_att.setModel(modelCast->m_att.get());
		m_hold.setModel(modelCast->m_hold.get());
		m_dec.setModel(modelCast->m_dec.get());
		m_sus.setModel(modelCast->m_sus.get());
	}
private:
	Knob m_amt;
	Knob m_exp;
	Knob m_stretch;
	Knob m_del;
	Knob m_att;
	Knob m_hold;
	Knob m_dec;
	Knob m_sus;
};

using Definition = HPDefinition<HPEnvModel>;

template<> Definition::HPDefinition(HPInstrument* instrument) :
		HPDefinitionBase(instrument)
{
		m_forbidsArguments = true;
}

template<> Definition::~HPDefinition() = default;

template<> string Definition::name() { return ENV_NAME; }

template<> shared_ptr<HPEnvModel> Definition::newNodeImpl() {
	return make_shared<HPEnvModel>(m_instrument);
}

template<> unique_ptr<HPNodeView> Definition::instantiateView(HPView* hpview) {
	return make_unique<HPEnvView>(hpview);
}

} // namespace lmms::hyperpipe
