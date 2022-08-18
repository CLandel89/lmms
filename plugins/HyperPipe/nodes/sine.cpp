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

inline unique_ptr<HPNode> instantiateSine(HPModel* model, int model_i);

struct HPSineModel : public HPModel::Node {
	HPSineModel(Instrument* instrument) :
			Node(instrument),
			m_sawify(0.0f, 0.0f, 1.0f, 0.01f, instrument, QString("sawify"))
	{}
	FloatModel m_sawify;
	unique_ptr<HPNode> instantiate(HPModel* model, int model_i) {
		return instantiateSine(model, model_i);
	}
	string name() {
		return SINE_NAME;
	}
	void load(int model_i, const QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_sawify.loadSettings(elem, is + "_sawify");
	}
	void save(int model_i, QDomDocument& doc, QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_sawify.saveSettings(doc, elem, is + "_sawify");
	}
};

class HPSine : public HPOsc
{
public:
	HPSine(HPModel* model, int model_i, HPSineModel* nmodel) :
			HPOsc(model, model_i),
			m_sawify(&nmodel->m_sawify)
	{}
private:
	float shape(float ph) {
		if (ph < 0.5f) {
			//this helps with FM detuning when using a "sawified" argument
			return -shape(1.0f - ph);
		}
		float s = sinf(ph * F_2PI);
		float sawify = m_sawify->value();
		float saw = ph; //simplified and reversed
		saw = 1.0f - sawify * saw; //ready for multiplication with s
		s *= saw;
		float attenuate = (1 - sawify) * 0.75f + sawify * 2.3f;
		return attenuate * s;
	}
	FloatModel* m_sawify;
};

inline unique_ptr<HPNode> instantiateSine(HPModel* model, int model_i) {
	return make_unique<HPSine>(
		model,
		model_i,
		static_cast<HPSineModel*>(model->m_nodes[model_i].get())
	);
}

class HPSineView : public HPNodeView {
public:
	HPSineView(HPView* view) :
			m_sawify(new Knob(view, "sawify"))
	{
		m_widgets.emplace_back(m_sawify);
	}
	void setModel(HPModel::Node* model) {
		HPSineModel *modelCast = static_cast<HPSineModel*>(model);
		m_sawify->setModel(&modelCast->m_sawify);
	}
private:
	Knob *m_sawify;
};

using Definition = HPDefinition<HPSineModel>;

template<> Definition::HPDefinition(HPInstrument* instrument) :
		HPDefinitionBase(instrument)
{}

template<> Definition::~HPDefinition() = default;

template<> string Definition::name() { return SINE_NAME; }

template<> unique_ptr<HPSineModel> Definition::newNodeImpl() {
	return make_unique<HPSineModel>(m_instrument);
}

template<> unique_ptr<HPNodeView> Definition::instantiateView(HPView* hpview) {
	return make_unique<HPSineView>(hpview);
}

} // namespace lmms::hyperpipe
