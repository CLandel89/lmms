/*
	tune.cpp - implementation of the "tune" node type

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

const string TUNE_NAME = "tune";

inline unique_ptr<HPNode> instantiateTune(HPModel* model, int model_i);

struct HPTuneModel : public HPModel::Node {
	HPTuneModel(Instrument* instrument) :
			Node(instrument),
			m_tones(0.0f, -100.0f, 100.0f, 0.01f, instrument, QString("tones")),
			m_nomin(1, 1, 999, instrument, QString("nominator")),
			m_denom(1, 1, 999, instrument, QString("denominator"))
	{}
	FloatModel m_tones;
	IntModel m_nomin;
	IntModel m_denom;
	unique_ptr<HPNode> instantiate(HPModel* model, int model_i) {
		return instantiateTune(model, model_i);
	}
	string name() { return TUNE_NAME; }
	void load(int model_i, const QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_tones.loadSettings(elem, is + "_tones");
		m_nomin.loadSettings(elem, is + "_nomin");
		m_denom.loadSettings(elem, is + "_denom");
	}
	void save(int model_i, QDomDocument& doc, QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_tones.saveSettings(doc, elem, is + "_tones");
		m_nomin.saveSettings(doc, elem, is + "_nomin");
		m_denom.saveSettings(doc, elem, is + "_denom");
	}
	bool usesPrev() { return true; }
};

class HPTune : public HPNode
{
public:
	HPTune(HPModel* model, int model_i, shared_ptr<HPTuneModel> nmodel) :
			m_nmodel(nmodel),
			m_prev(model->instantiatePrev(model_i))
	{}
private:
	float processFrame(Params p) {
		if (m_prev == nullptr) {
			return 0.0f;
		}
		if (! m_ph_valid) {
			m_ph = p.ph;
			m_ph_valid = true;
		}
		float tones = m_nmodel->m_tones.value();
		float nomin = m_nmodel->m_nomin.value();
		float denom = m_nmodel->m_denom.value();
		float tune = powf(2.0f, tones / 12.0f) * nomin / denom;
		// calculate own p.ph, adapt p.freq and p.freqMod
		p.ph = m_ph;
		p.freq *= tune;
		p.freqMod *= tune;
		m_ph += p.freqMod / p.srate;
		m_ph = hpposmodf(m_ph);
		return m_prev->processFrame(p);
	}
	void resetState() override {
		HPNode::resetState();
		m_ph_valid = false;
		if (m_prev != nullptr) {
			m_prev->resetState();
		}
	}
	float m_ph;
	bool m_ph_valid = false;
	shared_ptr<HPTuneModel> m_nmodel;
	unique_ptr<HPNode> m_prev;
};

inline unique_ptr<HPNode> instantiateTune(HPModel* model, int model_i) {
	return make_unique<HPTune>(
		model,
		model_i,
		static_pointer_cast<HPTuneModel>(model->m_nodes[model_i])
	);
}

class HPTuneView : public HPNodeView {
public:
	HPTuneView(HPView* view) :
			m_tones(new Knob(view, "tune")),
			m_nomin(new LcdSpinBox(3, view, "nominator")),
			m_denom(new LcdSpinBox(3, view, "denominator"))
	{
		m_widgets.emplace_back(m_tones);
		m_nomin->move(30, 0);
		m_widgets.emplace_back(m_nomin);
		m_denom->move(30, 30);
		m_widgets.emplace_back(m_denom);
	}
	void setModel(weak_ptr<HPModel::Node> nmodel) {
		auto modelCast = static_cast<HPTuneModel*>(nmodel.lock().get());
		m_tones->setModel(&modelCast->m_tones);
		m_nomin->setModel(&modelCast->m_nomin);
		m_denom->setModel(&modelCast->m_denom);
	}
private:
	Knob *m_tones;
	LcdSpinBox *m_nomin;
	LcdSpinBox *m_denom;
};

using Definition = HPDefinition<HPTuneModel>;

template<> Definition::HPDefinition(HPInstrument* instrument) :
		HPDefinitionBase(instrument)
{
	m_forbidsArguments = true;
}

template<> Definition::~HPDefinition() = default;

template<> string Definition::name() { return TUNE_NAME; }

template<> unique_ptr<HPTuneModel> Definition::newNodeImpl() {
	return make_unique<HPTuneModel>(m_instrument);
}

template<> unique_ptr<HPNodeView> Definition::instantiateView(HPView* hpview) {
	return make_unique<HPTuneView>(hpview);
}

} // namespace lmms::hyperpipe
