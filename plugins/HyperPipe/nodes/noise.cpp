/*
	noise.cpp - implementation of the "noise" node type

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

const string NOISE_NAME = "noise";

inline unique_ptr<HPNode> instantiateNoise(HPModel* model, int model_i);

struct HPNoiseModel : public HPModel::Node {
	HPNoiseModel(Instrument* instrument) :
			HPModel::Node(instrument),
			m_seed(1, 1, 0xffff, instrument, QString("seed"))
	{}
	IntModel m_seed;
	unique_ptr<HPNode> instantiate(HPModel* model, int model_i) {
		return instantiateNoise(model, model_i);
	}
	string name() { return NOISE_NAME; }
	void load(int model_i, const QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_seed.loadSettings(elem, is + "_seed");
	}
	void save(int model_i, QDomDocument& doc, QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_seed.saveSettings(doc, elem, is + "_seed");
	}
	bool usesPrev() { return false; }
};

class HPNoise : public HPNode
{
	float m_excess = 0;
	HPcbrng m_rng;
	uint32_t m_rng_counter = 0;
public:
	HPNoise(HPModel* model, int model_i, shared_ptr<HPNoiseModel> nmodel) :
			HPNode(),
			m_rng(nmodel->m_seed.value()),
			m_nmodel(nmodel)
	{}
	float processFrame(Params p) {
		float rval = m_rng(m_rng_counter++);
		float f1 = rval / 0xffff;
		float f = 1 - 2*f1 - m_excess / 4;
		if (f > 1) {
			f = 1;
		}
		if (f < -1) {
			f = -1;
		}
		m_excess += f;
		return f;
	}
	shared_ptr<HPNoiseModel> m_nmodel;
};

inline unique_ptr<HPNode> instantiateNoise(HPModel* model, int model_i) {
	return make_unique<HPNoise>(
		model,
		model_i,
		static_pointer_cast<HPNoiseModel>(model->m_nodes[model_i])
	);
}

class HPNoiseView : public HPNodeView {
public:
	HPNoiseView(HPView* view, HPInstrument* instrument) :
			m_seed(new LcdSpinBox(5, view, "seed"))
	{
		m_widgets.emplace_back(m_seed);
	}
	void setModel(weak_ptr<HPModel::Node> nmodel) {
		auto modelCast = static_cast<HPNoiseModel*>(nmodel.lock().get());
		m_seed->setModel(&modelCast->m_seed);
	}
private:
	LcdSpinBox *m_seed;
};

using Definition = HPDefinition<HPNoiseModel>;

template<> Definition::HPDefinition(HPInstrument* instrument) :
		HPDefinitionBase(instrument)
{
	m_forbidsArguments = true;
}

template<> Definition::~HPDefinition() = default;

template<> string Definition::name() { return NOISE_NAME; }

template<> unique_ptr<HPNoiseModel> Definition::newNodeImpl() {
	return make_unique<HPNoiseModel>(m_instrument);
}

template<> unique_ptr<HPNodeView> Definition::instantiateView(HPView* hpview) {
	return make_unique<HPNoiseView>(hpview, m_instrument);
}

} // namespace lmms::hyperpipe
