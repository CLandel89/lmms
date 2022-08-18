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
};

class HPOrganify : public HPNode
{
public:
	HPOrganify(HPModel* model, int model_i, HPOrganifyModel* nmodel) :
			m_tones(nmodel->m_tones.value()),
			m_weaken(&nmodel->m_weaken),
			m_prev(m_tones + 1 + m_tones)
	{
		for (int t = 0; t < m_tones + 1 + m_tones; t++) {
			m_prev[t] = model->instantiatePrev(model_i);
		}
	}
private:
	float processFrame(float freq, float srate) {
		if (m_prev[0] == nullptr) {
			return 0.0f;
		}
		float sumWeights = 0.0f;
		float result = 0.0f;
		for (int sub = 0; sub < m_tones; sub++) {
			float r = 2.0f / (3.0f + float(sub));
			float w = 1.0f - float(sub + 1) / (m_tones + 1);
			w = powf(w, m_weaken->value());
			sumWeights += w;
			result += w * m_prev[sub]->processFrame(r * freq, srate);
		}
		sumWeights += 1.0f;
		result += m_prev[m_tones]->processFrame(freq, srate);
		for (int over = 0; over < m_tones; over++) {
			float r = (3.0f + float(over)) / 2.0f;
			float w = 1.0f - float(over + 1) / (m_tones + 1);
			w = powf(w, m_weaken->value());
			sumWeights += w;
			result += w * m_prev[m_tones + 1 + over]->processFrame(r * freq, srate);
		}
		return result / sumWeights;
	}
	int m_tones;
	FloatModel *m_weaken;
	vector<unique_ptr<HPNode>> m_prev;
};

inline unique_ptr<HPNode> instantiateOrganify(HPModel* model, int model_i) {
	return make_unique<HPOrganify>(
		model,
		model_i,
		static_cast<HPOrganifyModel*>(model->m_nodes[model_i].get())
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
	void setModel(HPModel::Node *model) {
		HPOrganifyModel *modelCast = static_cast<HPOrganifyModel*>(model);
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
