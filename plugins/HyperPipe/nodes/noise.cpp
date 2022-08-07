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

inline unique_ptr<HPNode> instantiateNoise(shared_ptr<HPModel::Node> self);

struct HPNoiseModel : public HPModel::Node {
	HPNoiseModel(Instrument* instrument) :
			HPModel::Node(instrument),
			m_spike(make_shared<FloatModel>(4.0f, 0.0f, 20.0f, 0.1f, instrument, QString("spike")))
	{}
	shared_ptr<FloatModel> m_spike;
	unique_ptr<HPNode> instantiate(shared_ptr<HPModel::Node> self) {
		return instantiateNoise(self);
	}
	string name() {
		return NOISE_NAME;
	}
	void load(string params) {}
	string save() { return ""; }
};

class HPNoise : public HPNode
{
public:
	shared_ptr<FloatModel> m_spike = nullptr;
	float m_spike_fb = 4.0f;
	HPNoise(shared_ptr<HPNoiseModel> model) {
		if (model != nullptr) {
			m_spike = model->m_spike;
		}
	}
	float processFrame(float freq, float srate) {
		float spike = m_spike != nullptr ? m_spike->value() : m_spike_fb;
		float osc = 0.0f;
		size_t number = 0;
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
			osc = powf(osc, spike);
		}
		else {
			osc = 1.0f;
		}
		float r = 1.0f - fastRandf(2.0f);
		return osc * r;
	}
};

inline unique_ptr<HPNode> instantiateNoise(shared_ptr<HPModel::Node> self) {
	return make_unique<HPNoise>(
		static_pointer_cast<HPNoiseModel>(self)
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
