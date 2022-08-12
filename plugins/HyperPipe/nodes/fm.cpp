/*
	fm.cpp - implementation of the "fm" node type

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

const string FM_NAME = "fm";

inline unique_ptr<HPNode> instantiateFm(HPModel* model, int model_i);

struct HPFmModel : public HPModel::Node {
	HPFmModel(Instrument* instrument) :
			Node(instrument),
			m_amp(make_shared<FloatModel>(1.0f, 0.0f, 50.0f, 0.1f, instrument, QString("FM amp")))
	{}
	shared_ptr<FloatModel> m_amp;
	unique_ptr<HPNode> instantiate(HPModel* model, int model_i) {
		return instantiateFm(model, model_i);
	}
	string name() { return FM_NAME; }
	void load(int model_i, const QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_amp->loadSettings(elem, is + "_amp");
	}
	void save(int model_i, QDomDocument& doc, QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_amp->saveSettings(doc, elem, is + "_amp");
	}
};

class HPFm : public HPNode
{
public:
	HPFm(HPModel* model, int model_i, shared_ptr<HPFmModel> nmodel) :
			m_amp(nmodel->m_amp),
			m_prev(model->instantiatePrev(model_i)),
			m_arguments(model->instantiateArguments(model_i))
	{}
private:
	float processFrame(float freq, float srate) {
		if (m_prev == nullptr) {
			return 0.0f;
		}
		float mod = 0.0f;
		for (auto &argument : m_arguments) {
			mod += argument->processFrame(freq, srate);
		}
		mod *= m_amp->value();
		float prevFreq = (mod + 1.0f) * freq;
		return m_prev->processFrame(prevFreq, srate);
	}
	shared_ptr<FloatModel> m_amp;
	unique_ptr<HPNode> m_prev;
	vector<unique_ptr<HPNode>> m_arguments;
};

inline unique_ptr<HPNode> instantiateFm(HPModel* model, int model_i) {
	return make_unique<HPFm>(
		model,
		model_i,
		static_pointer_cast<HPFmModel>(model->m_nodes[model_i])
	);
}

class HPFmView : public HPNodeView {
public:
	HPFmView(HPView* view) :
			m_amp(view, "FM amp")
	{
		m_widgets.emplace_back(&m_amp);
	}
	void setModel(shared_ptr<HPModel::Node> model) {
		shared_ptr<HPFmModel> modelCast = static_pointer_cast<HPFmModel>(model);
		m_amp.setModel(modelCast->m_amp.get());
	}
private:
	Knob m_amp;
};

using Definition = HPDefinition<HPFmModel>;

template<> Definition::HPDefinition(HPInstrument* instrument) :
		HPDefinitionBase(instrument)
{}

template<> Definition::~HPDefinition() = default;

template<> string Definition::name() { return FM_NAME; }

template<> shared_ptr<HPFmModel> Definition::newNodeImpl() {
	return make_shared<HPFmModel>(m_instrument);
}

template<> unique_ptr<HPNodeView> Definition::instantiateView(HPView* hpview) {
	return make_unique<HPFmView>(hpview);
}

} // namespace lmms::hyperpipe
