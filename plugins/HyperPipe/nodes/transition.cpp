/*
	transition.cpp - implementation of the "transition" node type

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

const string TRANSITION_NAME = "transition";

inline unique_ptr<HPNode> instantiateTransition(HPModel* model, int model_i);

struct HPTransitionModel : public HPModel::Node {
	HPTransitionModel(Instrument* instrument) :
			Node(instrument),
			m_tS(0.0f, 0.0f, 2.0f, 0.01f, instrument, QString("transition seconds")),
			m_tB(1.0f, 0.0f, 10.0f, 0.001f, instrument, QString("transition beats")),
			m_attExp(2.0f, 0.01f, 20.0f, 0.01f, instrument, QString("transition attack exponent")),
			m_order(instrument, QString("transition order")),
			m_smooth(true, instrument, QString("transition smooth"))
	{
		for (int o = 0; o < N_ORDER_TYPES; o++) {
			switch (o) {
				case KEEP_LAST: m_order.addItem("keep last"); break;
				case PINGPONG: m_order.addItem("pingpong"); break;
				case LOOP: m_order.addItem("loop"); break;
				default:
					string msg = "Please give a label to " + o + string(" in HPTransitionModel::OrderTypes.");
					throw invalid_argument(msg);
			}
		}
	}
	FloatModel m_tS;
	FloatModel m_tB;
	FloatModel m_attExp;
	ComboBoxModel m_order;
	BoolModel m_smooth;
	enum OrderTypes {
		KEEP_LAST,
		PINGPONG,
		LOOP,
		N_ORDER_TYPES
	};
	unique_ptr<HPNode> instantiate(HPModel* model, int model_i) {
		return instantiateTransition(model, model_i);
	}
	string name() { return TRANSITION_NAME; }
	void load(int model_i, const QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_tS.loadSettings(elem, is + "_tS");
		m_tB.loadSettings(elem, is + "_tB");
		m_attExp.loadSettings(elem, is + "_attExp");
		m_order.loadSettings(elem, is + "_order");
		m_smooth.loadSettings(elem, is + "_smooth");
	}
	void save(int model_i, QDomDocument& doc, QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_tS.saveSettings(doc, elem, is + "_tS");
		m_tB.saveSettings(doc, elem, is + "_tB");
		m_attExp.saveSettings(doc, elem, is + "_attExp");
		m_order.saveSettings(doc, elem, is + "_order");
		m_smooth.saveSettings(doc, elem, is + "_smooth");
	}
	bool usesPrev() { return true; }
};

class HPTransition : public HPNode
{
public:
	HPTransition(HPModel* model, int model_i, shared_ptr<HPTransitionModel> nmodel) :
			m_nmodel(nmodel)
	{
		auto prev = model->instantiatePrev(model_i);
		if (prev != nullptr) {
			m_nodes.emplace_back(move(prev));
		}
		for (auto &argument : model->instantiateArguments(model_i)) {
			m_nodes.emplace_back(move(argument));
		}
	}
private:
	float processFrame(Params p) {
		if (m_nodes.size() == 0) {
			return 0.0f;
		}
		if (m_nodes.size() == 1) {
			return m_nodes[0]->processFrame(p);
		}
		// m_state describes the (fractional) number of past node transitions
		HPNode *from = nullptr, *to = nullptr; //these are set in the switch scope
		switch (m_nmodel->m_order.value()) {
			case HPTransitionModel::KEEP_LAST:
				if (m_state >= m_nodes.size() - 1) {
					from = to = m_nodes.back().get();
				}
				else {
					int i = m_state;
					from = m_nodes[i].get();
					to = m_nodes[i + 1].get();
				}
				break;
			case HPTransitionModel::PINGPONG: {
				int state = hpposmodi(m_state, 2 * m_nodes.size() - 2);
				if (state <= m_nodes.size() - 2) {
					from = m_nodes[state].get();
					to = m_nodes[state + 1].get();
				}
				else {
					//this range begins at state=size-1
					//and ends at, including, state=2·size-3
					int i = 2 * m_nodes.size() - (state + 2);
					from = m_nodes[i].get();
					to = m_nodes[i - 1].get();
				}
				break;
			}
			case HPTransitionModel::LOOP: {
				int i = hpposmodi(m_state, m_nodes.size());
				if (i <= m_nodes.size() - 2) {
					from = m_nodes[i].get();
					to = m_nodes[i + 1].get();
				}
				else {
					from = m_nodes.back().get();
					to = m_nodes[0].get();
				}
				break;
			}
			default:
				string msg = "It appears that there is no implementation for HyperPipe transition order type "
					+ m_nmodel->m_order.value() + string(".");
				throw invalid_argument(msg);
		}
		if (from == to) {
			return from->processFrame(p);
		}
		float interp = hpposmodf(m_state);
		if (m_nmodel->m_smooth.value()) {
			interp = hpsstep(interp);
		}
		interp = powf(interp, m_nmodel->m_attExp.value()); //transition attack exponent
		float spb = 60.0f / Engine::getSong()->getTempo(); //seconds per beat
		float tS = m_nmodel->m_tS.value();
		float tB = m_nmodel->m_tB.value();
		// calculate node samples
		float f = from->processFrame(p);
		float t = to->processFrame(p);
		// change state
		float dur = tS + spb * tB; //duration of a single transition
		if (dur != 0) {
			float sample_dur = 1.0f / p.srate / dur; //duration of a sample / dur
			if (fraction(m_state) + sample_dur >= 1.0f) {
				// "from" has fulfilled a transition.
				// Now is the best time to reset the state of "from".
				from->resetState();
			}
			m_state += sample_dur;
		}
		// return result
		return (1 - interp) * f + interp * t;
	}
	void resetState() override {
		HPNode::resetState();
		m_state = 0.0f;
		for (auto &node : m_nodes) {
			node->resetState();
		}
	}
	shared_ptr<HPTransitionModel> m_nmodel;
	vector<unique_ptr<HPNode>> m_nodes;
	float m_state = 0.0f;
};

inline unique_ptr<HPNode> instantiateTransition(HPModel* model, int model_i) {
	return make_unique<HPTransition>(
		model,
		model_i,
		static_pointer_cast<HPTransitionModel>(model->m_nodes[model_i])
	);
}

class HPTransitionView : public HPNodeView {
public:
	HPTransitionView(HPView* view) :
			m_tS(new Knob(view, "transition seconds")),
			m_tB(new Knob(view, "transition beats")),
			m_attExp(new Knob(view, "transition attack exponent")),
			m_order(new ComboBox(view, "transition order")),
			m_smooth(new LedCheckBox(view, "transition smooth"))
	{
		m_widgets.emplace_back(m_tS);
		m_tB->move(30, 0);
		m_widgets.emplace_back(m_tB);
		m_attExp->move(60, 0);
		m_widgets.emplace_back(m_attExp);
		m_order->move(90, 0);
		m_widgets.emplace_back(m_order);
		m_smooth->move(0, 30);
		m_widgets.emplace_back(m_smooth);
	}
	void setModel(weak_ptr<HPModel::Node> nmodel) {
		auto modelCast = static_cast<HPTransitionModel*>(nmodel.lock().get());
		m_tS->setModel(&modelCast->m_tS);
		m_tB->setModel(&modelCast->m_tB);
		m_attExp->setModel(&modelCast->m_attExp);
		m_order->setModel(&modelCast->m_order);
		m_smooth->setModel(&modelCast->m_smooth);
	}
private:
	Knob *m_tS;
	Knob *m_tB;
	Knob *m_attExp;
	ComboBox *m_order;
	LedCheckBox *m_smooth;
};

using Definition = HPDefinition<HPTransitionModel>;

template<> Definition::HPDefinition(HPInstrument* instrument) :
		HPDefinitionBase(instrument)
{}

template<> Definition::~HPDefinition() = default;

template<> string Definition::name() { return TRANSITION_NAME; }

template<> unique_ptr<HPTransitionModel> Definition::newNodeImpl() {
	return make_unique<HPTransitionModel>(m_instrument);
}

template<> unique_ptr<HPNodeView> Definition::instantiateView(HPView* hpview) {
	return make_unique<HPTransitionView>(hpview);
}

} // namespace lmms::hyperpipe
