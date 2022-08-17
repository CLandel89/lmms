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
			m_tS(make_shared<FloatModel>(0.0f, 0.0f, 2.0f, 0.01f, instrument, QString("transition seconds"))),
			m_tB(make_shared<FloatModel>(1.0f, 0.0f, 10.0f, 0.001f, instrument, QString("transition beats"))),
			m_attExp(make_shared<FloatModel>(10.0f, 0.01f, 20.0f, 0.01f, instrument, QString("transition attack exponent"))),
			m_order(make_shared<ComboBoxModel>(instrument, QString("transition order")))
	{
		for (int o = 0; o < N_ORDER_TYPES; o++) {
			switch (o) {
				case KEEP_LAST: m_order->addItem("keep last"); break;
				case PINGPONG: m_order->addItem("pingpong"); break;
				case LOOP: m_order->addItem("loop"); break;
				default:
					string msg = "Please give a label to " + o + string(" in HPTransitionModel::OrderTypes.");
					throw invalid_argument(msg);
			}
		}
	}
	shared_ptr<FloatModel> m_tS;
	shared_ptr<FloatModel> m_tB;
	shared_ptr<FloatModel> m_attExp;
	shared_ptr<ComboBoxModel> m_order;
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
		m_tS->loadSettings(elem, is + "_tS");
		m_tB->loadSettings(elem, is + "_tB");
		m_attExp->loadSettings(elem, is + "_attExp");
		m_order->loadSettings(elem, is + "_order");
	}
	void save(int model_i, QDomDocument& doc, QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_tS->saveSettings(doc, elem, is + "_tS");
		m_tB->saveSettings(doc, elem, is + "_tB");
		m_attExp->saveSettings(doc, elem, is + "_attExp");
		m_order->saveSettings(doc, elem, is + "_order");
	}
};

class HPTransition : public HPNode
{
public:
	HPTransition(HPModel* model, int model_i, shared_ptr<HPTransitionModel> nmodel) :
			m_tS(nmodel->m_tS),
			m_tB(nmodel->m_tB),
			m_attExp(nmodel->m_attExp),
			m_order(nmodel->m_order)
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
	float processFrame(float freq, float srate) {
		if (m_nodes.size() == 0) {
			return 0.0f;
		}
		if (m_nodes.size() == 1) {
			return m_nodes[0]->processFrame(freq, srate);
		}
		// m_state describes the (fractional) number of past node transitions
		HPNode *from = nullptr, *to = nullptr; //these are set in the switch scope
		switch (m_order->value()) {
			case HPTransitionModel::KEEP_LAST:
				if (m_state < 0) {
					// negative state values can result from FM
					if (m_state < -1) {
						//should never become that much, but for extreme cases:
						from = to = m_nodes.back().get();
					}
					else {
						from = m_nodes.back().get();
						to = m_nodes[0].get();
					}
				}
				else {
					if (m_state >= m_nodes.size() - 1) {
						from = to = m_nodes.back().get();
					}
					else {
						int i = m_state;
						from = m_nodes[i].get();
						to = m_nodes[i + 1].get();
					}
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
			}
				break;
			default:
				string msg = "It appears that there is no implementation for HyperPipe transition order type "
					+ m_order->value() + string(".");
				throw invalid_argument(msg);
		}
		float interp = absFraction(m_state);
		interp = powf(interp, m_attExp->value());
		const float spb = 60.0f / Engine::getSong()->getTempo(); //seconds per beat
		const float dur = m_tS->value() + spb * m_tB->value();
		if (dur != 0) {
			m_state += 1.0f / srate / dur;
		}
		if (from == to) {
			return from->processFrame(freq, srate);
		}
		float f = from->processFrame(freq, srate);
		float t = to->processFrame(freq, srate);
		return (1 - interp) * f + interp * t;
	}
	shared_ptr<FloatModel> m_tS;
	shared_ptr<FloatModel> m_tB;
	shared_ptr<FloatModel> m_attExp;
	shared_ptr<ComboBoxModel> m_order;
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
			m_tS(view, "transition seconds"),
			m_tB(view, "transition beats"),
			m_attExp(view, "transition attack exponent"),
			m_order(view, "transition order")
	{
		m_widgets.emplace_back(&m_tS);
		m_tB.move(30, 0);
		m_widgets.emplace_back(&m_tB);
		m_attExp.move(60, 0);
		m_widgets.emplace_back(&m_attExp);
		m_order.move(90, 0);
		m_widgets.emplace_back(&m_order);
	}
	void setModel(shared_ptr<HPModel::Node> model) {
		shared_ptr<HPTransitionModel> modelCast = static_pointer_cast<HPTransitionModel>(model);
		m_tS.setModel(modelCast->m_tS.get());
		m_tB.setModel(modelCast->m_tB.get());
		m_attExp.setModel(modelCast->m_attExp.get());
		m_order.setModel(modelCast->m_order.get());
	}
private:
	Knob m_tS;
	Knob m_tB;
	Knob m_attExp;
	ComboBox m_order;
};

using Definition = HPDefinition<HPTransitionModel>;

template<> Definition::HPDefinition(HPInstrument* instrument) :
		HPDefinitionBase(instrument)
{}

template<> Definition::~HPDefinition() = default;

template<> string Definition::name() { return TRANSITION_NAME; }

template<> shared_ptr<HPTransitionModel> Definition::newNodeImpl() {
	return make_shared<HPTransitionModel>(m_instrument);
}

template<> unique_ptr<HPNodeView> Definition::instantiateView(HPView* hpview) {
	return make_unique<HPTransitionView>(hpview);
}

} // namespace lmms::hyperpipe
