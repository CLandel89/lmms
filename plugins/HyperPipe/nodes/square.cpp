/*
	square.cpp - implementation of the "square" node type

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

const string SQUARE_NAME = "square";

inline unique_ptr<HPNode> instantiateSquare(HPModel* model, int model_i);

struct HPSquareModel : public HPModel::Node {
	HPSquareModel(Instrument* instrument) :
			Node(instrument),
			m_duty(0.5f, 0.0f, 1.0f, 0.01f, instrument, QString("duty cycle")),
			m_sstep(false, instrument, QString("square smoothstep"))
	{}
	FloatModel m_duty;
	BoolModel m_sstep;
	unique_ptr<HPNode> instantiate(HPModel* model, int model_i) {
		return instantiateSquare(model, model_i);
	}
	string name() {
		return SQUARE_NAME;
	}
	void load(int model_i, const QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_duty.loadSettings(elem, is + "_duty");
		m_sstep.loadSettings(elem, is + "_sstep");
	}
	void save(int model_i, QDomDocument& doc, QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_duty.saveSettings(doc, elem, is + "_duty");
		m_sstep.saveSettings(doc, elem, is + "_sstep");
	}
};

class HPSquare : public HPOsc
{
public:
	HPSquare(HPModel* model, int model_i, HPSquareModel* nmodel) :
			HPOsc(model, model_i),
			m_duty(&nmodel->m_duty),
			m_sstep(&nmodel->m_sstep)
	{}
private:
	float shape(float ph) {
		float d = m_duty->value();
		if (! m_sstep->value()) {
			return ph < d ? 1 : -1;
		}
		float smoothstepped;
		float p1 = 0.5 * d, p2 = d, p3 = 1 - 0.5 * (1 - d);
		if (ph < p2) {
			if (ph < p1) {
				smoothstepped = ph / p1;
			}
			else {
				smoothstepped = 1 - (ph - p1) / (p2 - p1);
			}
		}
		else {
			if (ph < p3) {
				smoothstepped = -(ph - p2) / (p3 - p2);
			}
			else {
				smoothstepped = -1 + (ph - p3) / (1 - p3);
			}
		}
		smoothstepped = hpsstep((smoothstepped + 1) / 2);
		smoothstepped = 2 * smoothstepped - 1;
		return smoothstepped;
	}
	FloatModel *m_duty;
	BoolModel *m_sstep;
};

inline unique_ptr<HPNode> instantiateSquare(HPModel* model, int model_i) {
	return make_unique<HPSquare>(
		model,
		model_i,
		static_cast<HPSquareModel*>(model->m_nodes[model_i].get())
	);
}

class HPSquareView : public HPNodeView {
public:
	HPSquareView(HPView* view) :
			m_duty(new Knob(view, "duty cylce")),
			m_sstep(new LedCheckBox(view, "square smoothstep"))
	{
		m_widgets.emplace_back(m_duty);
		m_sstep->move(30, 0);
		m_widgets.emplace_back(m_sstep);
	}
	void setModel(HPModel::Node* model) {
		HPSquareModel *modelCast = static_cast<HPSquareModel*>(model);
		m_duty->setModel(&modelCast->m_duty);
		m_sstep->setModel(&modelCast->m_sstep);
	}
private:
	Knob *m_duty;
	LedCheckBox *m_sstep;
};

using Definition = HPDefinition<HPSquareModel>;

template<> Definition::HPDefinition(HPInstrument* instrument) :
		HPDefinitionBase(instrument)
{}

template<> Definition::~HPDefinition() = default;

template<> string Definition::name() { return SQUARE_NAME; }

template<> unique_ptr<HPSquareModel> Definition::newNodeImpl() {
	return make_unique<HPSquareModel>(m_instrument);
}

template<> unique_ptr<HPNodeView> Definition::instantiateView(HPView* hpview) {
	return make_unique<HPSquareView>(hpview);
}

} // namespace lmms::hyperpipe
