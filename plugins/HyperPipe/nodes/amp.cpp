/*
	amp.cpp - implementation of the "amp" node type

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

const string AMP_NAME = "amp";

inline unique_ptr<HPNode> instantiateAmp(HPModel* model, int model_i);

struct HPAmpModel : public HPModel::Node {
	HPAmpModel(Instrument* instrument) :
			Node(instrument),
			m_amp(1.0f, -10.0f, 10.0f, 0.01f, instrument, QString("amp")),
			m_db(0.0f, -20.0f, 20.0f, 0.1f, instrument, QString("dB"))
	{}
	FloatModel m_amp;
	FloatModel m_db;
	unique_ptr<HPNode> instantiate(HPModel* model, int model_i) {
		return instantiateAmp(model, model_i);
	}
	string name() { return AMP_NAME; }
	void load(int model_i, const QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_amp.loadSettings(elem, is + "_amp");
		m_db.loadSettings(elem, is + "_db");
	}
	void save(int model_i, QDomDocument& doc, QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_amp.saveSettings(doc, elem, is + "_amp");
		m_db.saveSettings(doc, elem, is + "_db");
	}
	bool usesPrev() { return true; }
};

class HPAmp : public HPNode
{
public:
	HPAmp(HPModel* model, int model_i, shared_ptr<HPAmpModel> nmodel) :
			m_nmodel(nmodel),
			m_prev(model->instantiatePrev(model_i))
	{}
private:
	float processFrame(Params p) {
		if (m_prev == nullptr) {
			return 0.0f;
		}
		float amp = m_nmodel->m_amp.value();
		float db = m_nmodel->m_db.value();
		float a = amp * powf(10.0f, db / 20);
		return a * m_prev->processFrame(p);
	}
	void resetState() override {
		HPNode::resetState();
		if (m_prev != nullptr) {
			m_prev->resetState();
		}
	}
	shared_ptr<HPAmpModel> m_nmodel;
	unique_ptr<HPNode> m_prev = nullptr;
};

inline unique_ptr<HPNode> instantiateAmp(HPModel* model, int model_i) {
	return make_unique<HPAmp>(
		model,
		model_i,
		static_pointer_cast<HPAmpModel>(model->m_nodes[model_i])
	);
}

class HPAmpView : public HPNodeView {
public:
	HPAmpView(HPView* view) :
			m_amp(new Knob(view, "amp")),
			m_db(new Knob(view, "dB"))
	{
		m_widgets.emplace_back(m_amp);
		m_db->move(30, 0);
		m_widgets.emplace_back(m_db);
	}
	void setModel(weak_ptr<HPModel::Node> model) {
		auto modelCast = static_cast<HPAmpModel*>(model.lock().get());
		m_amp->setModel(&modelCast->m_amp);
		m_db->setModel(&modelCast->m_db);
	}
private:
	Knob *m_amp;
	Knob *m_db;
};

using Definition = HPDefinition<HPAmpModel>;

template<> Definition::HPDefinition(HPInstrument* instrument) :
		HPDefinitionBase(instrument)
{
		m_forbidsArguments = true;
}

template<> Definition::~HPDefinition() = default;

template<> string Definition::name() { return AMP_NAME; }

template<> unique_ptr<HPAmpModel> Definition::newNodeImpl() {
	return make_unique<HPAmpModel>(m_instrument);
}

template<> unique_ptr<HPNodeView> Definition::instantiateView(HPView* hpview) {
	return make_unique<HPAmpView>(hpview);
}

} // namespace lmms::hyperpipe