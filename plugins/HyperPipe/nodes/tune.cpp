/*
	tune.cpp - implementation of the "tune" node type

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

const string TUNE_NAME = "tune";

inline unique_ptr<HPNode> instantiateTune(HPModel* model, int model_i);

struct HPTuneModel : public HPModel::Node {
	HPTuneModel(Instrument* instrument) :
			Node(instrument),
			m_tune(0.0f, -100.0f, 100.0f, 0.01f, instrument, QString("tune"))
	{}
	FloatModel m_tune;
	unique_ptr<HPNode> instantiate(HPModel* model, int model_i) {
		return instantiateTune(model, model_i);
	}
	string name() { return TUNE_NAME; }
	void load(int model_i, const QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_tune.loadSettings(elem, is + "_tune");
	}
	void save(int model_i, QDomDocument& doc, QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_tune.saveSettings(doc, elem, is + "_tune");
	}
};

class HPTune : public HPNode
{
public:
	HPTune(HPModel* model, int model_i, HPTuneModel* nmodel) :
			m_tune(&nmodel->m_tune),
			m_prev(model->instantiatePrev(model_i))
	{}
private:
	float processFrame(float freq, float srate) {
		if (m_prev == nullptr) {
			return 0.0f;
		}
		float tune = m_tune->value();
		tune = powf(2.0f, tune / 12.0f);
		return m_prev->processFrame(tune * freq, srate);
	}
	FloatModel *m_tune;
	unique_ptr<HPNode> m_prev;
};

inline unique_ptr<HPNode> instantiateTune(HPModel* model, int model_i) {
	return make_unique<HPTune>(
		model,
		model_i,
		static_cast<HPTuneModel*>(model->m_nodes[model_i].get())
	);
}

class HPTuneView : public HPNodeView {
public:
	HPTuneView(HPView* view) :
			m_tune(new Knob(view, "tune"))
	{
		m_widgets.emplace_back(m_tune);
	}
	void setModel(HPModel::Node* model) {
		HPTuneModel *modelCast = static_cast<HPTuneModel*>(model);
		m_tune->setModel(&modelCast->m_tune);
	}
private:
	Knob *m_tune;
};

using Definition = HPDefinition<HPTuneModel>;

template<> Definition::HPDefinition(HPInstrument* instrument) :
		HPDefinitionBase(instrument)
{
	m_forbidsArguments = true;
}

template<> Definition::~HPDefinition() = default;

template<> string Definition::name() { return TUNE_NAME; }

template<> unique_ptr<HPTuneModel> Definition::newNodeImpl() {
	return make_unique<HPTuneModel>(m_instrument);
}

template<> unique_ptr<HPNodeView> Definition::instantiateView(HPView* hpview) {
	return make_unique<HPTuneView>(hpview);
}

} // namespace lmms::hyperpipe
