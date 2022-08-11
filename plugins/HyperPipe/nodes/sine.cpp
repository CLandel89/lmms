/*
	sine.cpp - implementation of the "sine" node type

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

const string SINE_NAME = "sine";

inline unique_ptr<HPNode> instantiateSine(shared_ptr<HPModel::Node> self);

struct HPSineModel : public HPModel::Node {
	HPSineModel(Instrument* instrument) :
			Node(instrument),
			m_sawify(make_shared<FloatModel>(0.0f, 0.0f, 1.0f, 0.01f, instrument, QString("sawify")))
	{}
	shared_ptr<FloatModel> m_sawify;
	unique_ptr<HPNode> instantiate(shared_ptr<HPModel::Node> self) {
		return instantiateSine(self);
	}
	string name() {
		return SINE_NAME;
	}
	void load(string params) {}
	string save() { return ""; }
};

class HPSine : public HPOsc
{
public:
	HPSine(shared_ptr<HPSineModel> model) {
		if (model != nullptr) {
			m_sawify = model->m_sawify;
		}
	}
	shared_ptr<FloatModel> m_sawify = nullptr;
	float m_sawify_fb = 0.0f; //fallback if no model
private:
	float shape(float ph) {
		if (ph < 0.5f) {
			//this helps with FM detuning when using a "sawified" argument
			return -shape(1.0f - ph);
		}
		float sawify = m_sawify != nullptr ? m_sawify->value() : m_sawify_fb;
		float s = sinf(ph * F_2PI);
		float saw = ph; //simplified and reversed
		saw = 1.0f - sawify * saw; //ready for multiplication with s
		return saw * s;
	}
};

inline unique_ptr<HPNode> instantiateSine(shared_ptr<HPModel::Node> self) {
	return make_unique<HPSine>(
		static_pointer_cast<HPSineModel>(self)
	);
}

class HPSineView : public HPNodeView {
public:
	HPSineView(HPView* view) :
			m_sawify(view, "sawify")
	{
		m_widgets.emplace_back(&m_sawify);
	}
	void setModel(shared_ptr<HPModel::Node> model) {
		shared_ptr<HPSineModel> modelCast = static_pointer_cast<HPSineModel>(model);
		m_sawify.setModel(modelCast->m_sawify.get());
	}
private:
	Knob m_sawify;
};

using Definition = HPDefinition<HPSineModel>;

template<> Definition::HPDefinition(HPInstrument* instrument) :
		HPDefinitionBase(instrument)
{}

template<> Definition::~HPDefinition() = default;

template<> string Definition::name() { return SINE_NAME; }

template<> shared_ptr<HPSineModel> Definition::newNodeImpl() {
	return make_shared<HPSineModel>(m_instrument);
}

template<> unique_ptr<HPNodeView> Definition::instantiateView(HPView* hpview) {
	return make_unique<HPSineView>(hpview);
}

} // namespace lmms::hyperpipe
