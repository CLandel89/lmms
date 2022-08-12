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
			m_tones(make_shared<IntModel>(1, 1, 9, instrument, QString("tones"))),
			m_weaken(make_shared<FloatModel>(1.0f, 0.0f, 10.0f, 0.1f, instrument, QString("weaken")))
	{}
	shared_ptr<IntModel> m_tones;
	shared_ptr<FloatModel> m_weaken;
	unique_ptr<HPNode> instantiate(HPModel* model, int model_i) {
		return instantiateOrganify(model, model_i);
	}
	string name() { return ORGANIFY_NAME; }
	void load(int model_i, const QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_tones->loadSettings(elem, is + "_tones");
		m_weaken->loadSettings(elem, is + "_weaken");
	}
	void save(int model_i, QDomDocument& doc, QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_tones->saveSettings(doc, elem, is + "_tones");
		m_weaken->saveSettings(doc, elem, is + "_weaken");
	}
};

class HPOrganify : public HPNode
{
public:
	HPOrganify(HPModel* model, int model_i, shared_ptr<HPOrganifyModel> nmodel) :
			m_tones(nmodel->m_tones->value()),
			m_weaken(nmodel->m_weaken)
	{
		for (int t = 0; t < m_tones + 1 + m_tones; t++) {
			m_prev.emplace_back(model->instantiatePrev(model_i));
			m_arguments.emplace_back(model->instantiateArguments(model_i));
		}
	}
private:
	float processFrame(float freq, float srate) {
		int inputNumber = m_prev[0] != nullptr ? 1 : 0;
		if (! m_arguments.empty()) {
			inputNumber += m_arguments[0].size();
		}
		if (inputNumber == 0) {
			return 0.0f;
		}
		float sumWeights = 0.0f;
		float result = 0.0f;
		// "current pipe"
		if (m_prev[0] != nullptr) {
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
		}
		// "argument pipes"
		for (int sub = 0; sub < m_tones; sub++) {
			float r = 2.0f / (3.0f + float(sub));
			float w = 1.0f - float(sub + 1) / (m_tones + 1);
			w = powf(w, m_weaken->value());
			for (auto &argument : m_arguments[sub]) {
				sumWeights += w;
				result += w * argument->processFrame(r * freq, srate);
			}
		}
		for (auto &argument : m_arguments[m_tones]) {
			sumWeights += 1.0f;
			result += argument->processFrame(freq, srate);
		}
		for (int over = 0; over < m_tones; over++) {
			float r = (3.0f + float(over)) / 2.0f;
			float w = 1.0f - float(over + 1) / (m_tones + 1);
			w = powf(w, m_weaken->value());
			for (auto &argument : m_arguments[m_tones + 1 + over]) {
				sumWeights += w;
				result += w * argument->processFrame(r * freq, srate);
			}
		}
		return result / sumWeights;
	}
	int m_tones;
	shared_ptr<FloatModel> m_weaken;
	vector<unique_ptr<HPNode>> m_prev;
	vector<vector<unique_ptr<HPNode>>> m_arguments;
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
			m_tones(1, view, "tones"),
			m_weaken(view, "weaken")
	{
		m_widgets.emplace_back(&m_tones);
		m_weaken.move(25, 0);
		m_widgets.emplace_back(&m_weaken);
	}
	void setModel(shared_ptr<HPModel::Node> model) {
		shared_ptr<HPOrganifyModel> modelCast = static_pointer_cast<HPOrganifyModel>(model);
		m_tones.setModel(modelCast->m_tones.get());
		m_weaken.setModel(modelCast->m_weaken.get());
	}
private:
	LcdSpinBox m_tones;
	Knob m_weaken;
};

using Definition = HPDefinition<HPOrganifyModel>;

template<> Definition::HPDefinition(HPInstrument* instrument) :
		HPDefinitionBase(instrument)
{}

template<> Definition::~HPDefinition() = default;

template<> string Definition::name() { return ORGANIFY_NAME; }

template<> shared_ptr<HPOrganifyModel> Definition::newNodeImpl() {
	return make_shared<HPOrganifyModel>(m_instrument);
}

template<> unique_ptr<HPNodeView> Definition::instantiateView(HPView* hpview) {
	return make_unique<HPOrganifyView>(hpview);
}

} // namespace lmms::hyperpipe
