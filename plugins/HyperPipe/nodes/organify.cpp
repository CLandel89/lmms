/*
	organify.cpp - implementation of the "organify" node type

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

const string ORGANIFY_NAME = "organify";

inline unique_ptr<HPNode> instantiateOrganify(HPModel* model, int model_i);

struct HPOrganifyModel : public HPModel::Node {
	HPOrganifyModel(Instrument* instrument) :
			Node(instrument),
			m_tones(1, 1, 9, instrument, QString("tones")),
			m_weaken(1.0f, -5.0f, 10.0f, 0.1f, instrument, QString("weaken"))
	{}
	IntModel m_tones;
	FloatModel m_weaken;
	unique_ptr<HPNode> instantiate(HPModel* model, int model_i) {
		return instantiateOrganify(model, model_i);
	}
	string name() { return ORGANIFY_NAME; }
	void load(int model_i, const QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_tones.loadSettings(elem, is + "_tones");
		m_weaken.loadSettings(elem, is + "_weaken");
	}
	void save(int model_i, QDomDocument& doc, QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_tones.saveSettings(doc, elem, is + "_tones");
		m_weaken.saveSettings(doc, elem, is + "_weaken");
	}
	bool usesPrev() { return true; }
};

class HPOrganify : public HPNode
{
public:
	HPOrganify(HPModel* model, int model_i, shared_ptr<HPOrganifyModel> nmodel) :
			m_nmodel(nmodel),
			m_tones(nmodel->m_tones.value()),
			m_ph(m_tones + 1 + m_tones),
			m_prev(m_tones + 1 + m_tones)
	{
		for (int t = 0; t < m_tones + 1 + m_tones; t++) {
			m_prev[t] = model->instantiatePrev(model_i);
		}
	}
private:
	float processFrame(Params p) {
		if (m_prev[0] == nullptr) {
			return 0.0f;
		}
		if (! m_ph_initted) {
			for (int i = 0; i < m_tones + 1 + m_tones; i++) {
				m_ph[i] = p.ph;
			}
			m_ph_initted = true;
		}
		float weaken = m_nmodel->m_weaken.value();
		float sumWeights = 0.0f;
		float result = 0.0f;
		for (float sub = 0; sub < m_tones; sub++) {
			Params ps = p;
			float r = 2.0f / (3.0f + sub);
			// re-calculate ps.ph, adapt ps.freq, ps.freqMod and m_ph
			ps.freq *= r;
			ps.freqMod *= r;
			ps.ph = m_ph[sub];
			m_ph[sub] += ps.freqMod / ps.srate;
			m_ph[sub] = hpposmodf(m_ph[sub]);
			float w = 1.0f - float(sub + 1) / (m_tones + 1);
			w = powf(w, weaken);
			sumWeights += w;
			result += w * m_prev[sub]->processFrame(ps);
		}
		p.ph = m_ph[m_tones]; // consistent with the sub and over tones
		m_ph[m_tones] += p.freqMod / p.srate;
		m_ph[m_tones] = hpposmodf(m_ph[m_tones]);
		sumWeights += 1.0f;
		result += m_prev[m_tones]->processFrame(p);
		for (float over = 0; over < m_tones; over++) {
			Params po = p;
			float r = (3.0f + over) / 2.0f;
			// re-calculate po.ph, adapt po.freq, po.freqMod and m_ph
			po.freq *= r;
			po.freqMod *= r;
			po.ph = m_ph[m_tones + 1 + over];
			m_ph[m_tones + 1 + over] += po.freqMod / po.srate;
			m_ph[m_tones + 1 + over] = hpposmodf(m_ph[m_tones + 1 + over]);
			float w = 1.0f - (over + 1.0f) / (m_tones + 1);
			w = powf(w, weaken);
			sumWeights += w;
			result += w * m_prev[m_tones + 1 + over]->processFrame(po);
		}
		return result / sumWeights;
	}
	void resetState() override {
		HPNode::resetState();
		m_ph_initted = false;
		for (auto &prev : m_prev) {
			prev->resetState();
		}
	}
	shared_ptr<HPOrganifyModel> m_nmodel;
	int m_tones;
	vector<float> m_ph;
	bool m_ph_initted = false;
	vector<unique_ptr<HPNode>> m_prev;
};

inline unique_ptr<HPNode> instantiateOrganify(HPModel* model, int model_i) {
	return make_unique<HPOrganify>(
		model,
		model_i,
		static_pointer_cast<HPOrganifyModel>(model->m_nodes[model_i])
	);
}

class HPOrganifyView : public HPNodeView {
public:
	HPOrganifyView(HPView* view) :
			m_tones(new LcdSpinBox(1, view, "tones")),
			m_weaken(new Knob(view, "weaken"))
	{
		m_widgets.emplace_back(m_tones);
		m_weaken->move(25, 0);
		m_widgets.emplace_back(m_weaken);
	}
	void setModel(weak_ptr<HPModel::Node> nmodel) {
		auto modelCast = static_cast<HPOrganifyModel*>(nmodel.lock().get());
		m_tones->setModel(&modelCast->m_tones);
		m_weaken->setModel(&modelCast->m_weaken);
	}
private:
	LcdSpinBox *m_tones;
	Knob *m_weaken;
};

using Definition = HPDefinition<HPOrganifyModel>;

template<> Definition::HPDefinition(HPInstrument* instrument) :
		HPDefinitionBase(instrument)
{
		m_forbidsArguments = true;
}

template<> Definition::~HPDefinition() = default;

template<> string Definition::name() { return ORGANIFY_NAME; }

template<> unique_ptr<HPOrganifyModel> Definition::newNodeImpl() {
	return make_unique<HPOrganifyModel>(m_instrument);
}

template<> unique_ptr<HPNodeView> Definition::instantiateView(HPView* hpview) {
	return make_unique<HPOrganifyView>(hpview);
}

} // namespace lmms::hyperpipe
