/*
	mix.cpp - implementation of the "mix" node type

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

const string MIX_NAME = "mix";

inline unique_ptr<HPNode> instantiateMix(HPModel* model, int model_i);

struct HPMixModel : public HPModel::Node {
	HPMixModel(Instrument* instrument) :
			Node(instrument),
			m_mix(0.0f, 0.0f, 1.0f, 0.01f, instrument, QString("mix"))
	{}
	FloatModel m_mix;
	unique_ptr<HPNode> instantiate(HPModel* model, int model_i) {
		return instantiateMix(model, model_i);
	}
	string name() { return MIX_NAME; }
	void load(int model_i, const QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_mix.loadSettings(elem, is + "_mix");
	}
	void save(int model_i, QDomDocument& doc, QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_mix.saveSettings(doc, elem, is + "_mix");
	}
	bool usesPrev() { return true; }
};

class HPMix : public HPNode
{
public:
	HPMix(HPModel* model, int model_i, shared_ptr<HPMixModel> nmodel) :
			m_nmodel(nmodel),
			m_prev(model->instantiatePrev(model_i)),
			m_arguments(model->instantiateArguments(model_i))
	{}
private:
	float processFrame(Params p) {
		float prev = 0.0f;
		if (m_prev != nullptr) {
			prev = m_prev->processFrame(p);
		}
		float args = 0.0f;
		for (auto &argument : m_arguments) {
			args += argument->processFrame(p);
		}
		if (m_arguments.size() > 0) {
			args /= m_arguments.size();
		}
		float mix = m_nmodel->m_mix.value();
		return (1 - mix) * prev + mix * args;
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
	shared_ptr<HPMixModel> m_nmodel;
	unique_ptr<HPNode> m_prev;
	vector<unique_ptr<HPNode>> m_arguments;
};

inline unique_ptr<HPNode> instantiateMix(HPModel* model, int model_i) {
	return make_unique<HPMix>(
		model,
		model_i,
		static_pointer_cast<HPMixModel>(model->m_nodes[model_i])
	);
}

class HPMixView : public HPNodeView {
public:
	HPMixView(HPView* view) :
			m_mix(new Knob(view, "mix"))
	{
		m_widgets.emplace_back(m_mix);
	}
	void setModel(weak_ptr<HPModel::Node> nmodel) {
		auto modelCast = static_cast<HPMixModel*>(nmodel.lock().get());
		m_mix->setModel(&modelCast->m_mix);
	}
private:
	Knob *m_mix;
};

using Definition = HPDefinition<HPMixModel>;

template<> Definition::HPDefinition(HPInstrument* instrument) :
		HPDefinitionBase(instrument)
{}

template<> Definition::~HPDefinition() = default;

template<> string Definition::name() { return MIX_NAME; }

template<> unique_ptr<HPMixModel> Definition::newNodeImpl() {
	return make_unique<HPMixModel>(m_instrument);
}

template<> unique_ptr<HPNodeView> Definition::instantiateView(HPView* hpview) {
	return make_unique<HPMixView>(hpview);
}

} // namespace lmms::hyperpipe