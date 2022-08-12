/*
	noise.cpp - implementation of the "noise" node type

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

const string NOISE_NAME = "noise";

inline unique_ptr<HPNode> instantiateNoise(HPModel* model, int model_i);

struct HPNoiseModel : public HPModel::Node {
	HPNoiseModel(Instrument* instrument) :
			HPModel::Node(instrument),
			m_spike(make_shared<FloatModel>(4.0f, 0.0f, 20.0f, 0.1f, instrument, QString("spike")))
	{}
	shared_ptr<FloatModel> m_spike;
	unique_ptr<HPNode> instantiate(HPModel* model, int model_i) {
		return instantiateNoise(model, model_i);
	}
	string name() {
		return NOISE_NAME;
	}
	void load(int model_i, const QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_spike->loadSettings(elem, is + "_spike");
	}
	void save(int model_i, QDomDocument& doc, QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_spike->saveSettings(doc, elem, is + "_spike");
	}
};

class HPNoise : public HPNode
{
public:
	HPNoise(HPModel* model, int model_i, shared_ptr<HPNoiseModel> nmodel) :
			m_spike(nmodel->m_spike),
			m_prev(model->instantiatePrev(model_i)),
			m_arguments(model->instantiateArguments(model_i))
	{}
	float processFrame(float freq, float srate) {
		float makeupAmp = 0.5f + 0.33f * m_spike->value();
		float osc = 0.0f;
		int number = 0;
		if (m_prev != nullptr) {
			osc = m_prev->processFrame(freq, srate);
			number = 1;
		}
		for (auto& argument : m_arguments) {
			osc += argument->processFrame(freq, srate);
			number++;
		}
		if (number > 0) {
			osc /= number;
			osc = (osc + 1.0f) / 2.0f; //0.0...1.0
			osc = powf(osc, m_spike->value());
		}
		else {
			makeupAmp = 0.5f;
			osc = 1.0f;
		}
		float r = 1.0f - fastRandf(2.0f);
		return makeupAmp * osc * r;
	}
private:
	shared_ptr<FloatModel> m_spike;
	unique_ptr<HPNode> m_prev;
	vector<unique_ptr<HPNode>> m_arguments;
};

inline unique_ptr<HPNode> instantiateNoise(HPModel* model, int model_i) {
	return make_unique<HPNoise>(
		model,
		model_i,
		static_pointer_cast<HPNoiseModel>(model->m_nodes[model_i])
	);
}

class HPNoiseView : public HPNodeView {
public:
	HPNoiseView(HPView* view, HPInstrument* instrument) :
			m_spike(view, "spike")
	{
		m_widgets.emplace_back(&m_spike);
	}
	void setModel(shared_ptr<HPModel::Node> model) {
		shared_ptr<HPNoiseModel> modelCast = static_pointer_cast<HPNoiseModel>(model);
		m_spike.setModel(modelCast->m_spike.get());
	}
private:
	Knob m_spike;
};

using Definition = HPDefinition<HPNoiseModel>;

template<> Definition::HPDefinition(HPInstrument* instrument) :
		HPDefinitionBase(instrument)
{}

template<> Definition::~HPDefinition() = default;

template<> string Definition::name() { return NOISE_NAME; }

template<> shared_ptr<HPNoiseModel> Definition::newNodeImpl() {
	return make_shared<HPNoiseModel>(m_instrument);
}

template<> unique_ptr<HPNodeView> Definition::instantiateView(HPView* hpview) {
	return make_unique<HPNoiseView>(hpview, m_instrument);
}

} // namespace lmms::hyperpipe
