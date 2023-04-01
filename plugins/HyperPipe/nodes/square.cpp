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

struct HPSquareModel : public HPOscModel {
	HPSquareModel(Instrument* instrument) :
			HPOscModel(instrument),
			m_duty(0.5f, 0.01f, 0.99f, 0.01f, instrument, QString("duty cycle")),
			m_ofree(true, instrument, QString("square offset-free")),
			m_sstep(false, instrument, QString("square smoothstep"))
	{}
	FloatModel m_duty;
	BoolModel m_ofree;
	BoolModel m_sstep;
	unique_ptr<HPNode> instantiate(HPModel* model, int model_i) {
		return instantiateSquare(model, model_i);
	}
	string name() {
		return SQUARE_NAME;
	}
	void loadImpl(int model_i, const QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_duty.loadSettings(elem, is + "_duty");
		m_ofree.loadSettings(elem, is + "_ofree");
		m_sstep.loadSettings(elem, is + "_sstep");
	}
	void saveImpl(int model_i, QDomDocument& doc, QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_duty.saveSettings(doc, elem, is + "_duty");
		m_ofree.saveSettings(doc, elem, is + "_ofree");
		m_sstep.saveSettings(doc, elem, is + "_sstep");
	}
};

class HPSquare : public HPOsc
{
public:
	HPSquare(HPModel* model, int model_i, shared_ptr<HPSquareModel> nmodel) :
			HPOsc(model, model_i, nmodel),
			m_nmodel(nmodel)
	{}
private:
	float shape(float ph) {
		float d = m_nmodel->m_duty.value();
		float d_ = 1 - d;
		float a = 1, b = -1; //amplitudes
		if (m_nmodel->m_ofree.value()) {
			// b or a must be adjusted in a manner that leads to: d·a = -d_·b
			// (It can be shown that these factors work as well when smoothstepping).
			if (d < 0.5) {
				// a = -d_·b / d
				// (b=-1)
				// a = -(1-d)(-1) / d
				a = (1 - d) / d;
			}
			else {
				// b = d·a / (-d_)
				// (a = 1)
				// b = -d/d_
				// (d = 1 - d_)
				// b = -(1-d_) / d_
				b = (d_ - 1) / d_;
			}
			float amp = 1 / (d*a - d_*b); // such that amp · (d·a - d_·b) = 1
			amp = powf(amp, 2); // full compensation sounds too loud
			a *= amp;
			b *= amp;
		}
		if (! m_nmodel->m_sstep.value()) {
			return ph < d ? a : b;
		}
		float p1 = 0.5 * d, p2 = d, p3 = 1 - 0.5*d_, p4 = 1;
		if (ph < p2) {
			if (ph < p1) {
				float result = ph / p1;
				result = hpsstep(result);
				return result * a; // 0.0...a
			}
			else {
				float result = 1 - (ph - p1) / (p2 - p1);
				result = hpsstep(result);
				return result * a; // a...0.0
			}
		}
		else {
			if (ph < p3) {
				float result = (ph - p2) / (p3 - p2);
				result = hpsstep(result);
				return result * b; // 0.0...b
			}
			else {
				float result = 1 - (ph - p3) / (p4 - p3);
				result = hpsstep(result);
				return result * b; // b...0.0
			}
		}
	}
	shared_ptr<HPSquareModel> m_nmodel;
};

inline unique_ptr<HPNode> instantiateSquare(HPModel* model, int model_i) {
	return make_unique<HPSquare>(
		model,
		model_i,
		static_pointer_cast<HPSquareModel>(model->m_nodes[model_i])
	);
}

class HPSquareView : public HPNodeView {
public:
	HPSquareView(HPView* view) :
			m_duty(new Knob(view, "duty cylce")),
			m_ofree(new LedCheckBox(view, "square offset-free")),
			m_sstep(new LedCheckBox(view, "square smoothstep"))
	{
		m_widgets.emplace_back(m_duty);
		m_ofree->move(30, 0);
		m_widgets.emplace_back(m_ofree);
		m_sstep->move(50, 0);
		m_widgets.emplace_back(m_sstep);
	}
	void setModel(weak_ptr<HPModel::Node> nmodel) {
		auto modelCast = static_cast<HPSquareModel*>(nmodel.lock().get());
		m_duty->setModel(&modelCast->m_duty);
		m_ofree->setModel(&modelCast->m_ofree);
		m_sstep->setModel(&modelCast->m_sstep);
	}
private:
	Knob *m_duty;
	LedCheckBox *m_ofree;
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
