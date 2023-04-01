/*
	am.cpp - implementation of the "am" node type

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

const string AM_NAME = "am";

inline unique_ptr<HPNode> instantiateAm(HPModel* model, int model_i);

struct HPAmModel : public HPModel::Node {
	HPAmModel(Instrument* instrument) :
			Node(instrument),
			m_amt(0.5f, 0.0f, 2.0f, 0.01f, instrument, QString("AM amount"))
	{}
	FloatModel m_amt;
	unique_ptr<HPNode> instantiate(HPModel* model, int model_i) {
		return instantiateAm(model, model_i);
	}
	string name() { return AM_NAME; }
	void load(int model_i, const QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_amt.loadSettings(elem, is + "_amt");
	}
	void save(int model_i, QDomDocument& doc, QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_amt.saveSettings(doc, elem, is + "_amt");
	}
	bool usesPrev() { return true; }
};

class HPAm : public HPNode
{
public:
	HPAm(HPModel* model, int model_i, shared_ptr<HPAmModel> nmodel) :
			m_nmodel(nmodel),
			m_prev(model->instantiatePrev(model_i)),
			m_arguments(model->instantiateArguments(model_i))
	{}
private:
	float processFrame(Params p) {
		if (m_prev == nullptr) {
			return 0.0f;
		}
		float result = m_prev->processFrame(p);
		for (auto &argument : m_arguments) {
			float a = argument->processFrame(p); // -1.0...1.0
			a += 1.0f; // 0.0...2.0
			a /= 2.0f; // 0.0...1.0
			float amt = m_nmodel->m_amt.value();
			a = (1.0f - amt) +  amt * a; // (1.0-amt)...1.0
			result *= a;
		}
		return result;
	}
	void resetState() override {
		HPNode::resetState();
		if (m_prev != nullptr) {
			m_prev->resetState();
		}
		for (auto &argument : m_arguments) {
			argument->resetState();
		}
	}
	shared_ptr<HPAmModel> m_nmodel;
	unique_ptr<HPNode> m_prev;
	vector<unique_ptr<HPNode>> m_arguments;
};

inline unique_ptr<HPNode> instantiateAm(HPModel* model, int model_i) {
	return make_unique<HPAm>(
		model,
		model_i,
		static_pointer_cast<HPAmModel>(model->m_nodes[model_i])
	);
}

class HPAmView : public HPNodeView {
public:
	HPAmView(HPView* view) :
			m_amt(new Knob(view, "AM amount"))
	{
		m_widgets.emplace_back(m_amt);
	}
	void setModel(weak_ptr<HPModel::Node> nmodel) {
		auto modelCast = static_cast<HPAmModel*>(nmodel.lock().get());
		m_amt->setModel(&modelCast->m_amt);
	}
private:
	Knob *m_amt;
};

using Definition = HPDefinition<HPAmModel>;

template<> Definition::HPDefinition(HPInstrument* instrument) :
		HPDefinitionBase(instrument)
{}

template<> Definition::~HPDefinition() = default;

template<> string Definition::name() { return AM_NAME; }

template<> unique_ptr<HPAmModel> Definition::newNodeImpl() {
	return make_unique<HPAmModel>(m_instrument);
}

template<> unique_ptr<HPNodeView> Definition::instantiateView(HPView* hpview) {
	return make_unique<HPAmView>(hpview);
}

} // namespace lmms::hyperpipe
