/*
	noiseDet.cpp - implementation of the "deterministic noise" node type

	HyperPipe - synth with arbitrary possibilities

	Copyright (c) 2024 Christian Landel

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

const string NOISE_DET_NAME = "noise det";

inline unique_ptr<HPNode> instantiateNoiseDet(HPModel* model, int model_i);

struct HPNoiseDetModel : public HPModel::Node {
	HPNoiseDetModel(Instrument* instrument) :
			HPModel::Node(instrument)
	{}
	unique_ptr<HPNode> instantiate(HPModel* model, int model_i) {
		return instantiateNoiseDet(model, model_i);
	}
	string name() { return NOISE_DET_NAME; }
	void load(int model_i, const QDomElement& elem) {}
	void save(int model_i, QDomDocument& doc, QDomElement& elem) {}
	bool usesPrev() { return false; }
};

class HPNoiseDet : public HPNode
{
	float m_iter = 0;
public:
	HPNoiseDet(HPModel* model, int model_i, shared_ptr<HPNoiseDetModel> nmodel) :
			HPNode()
	{}
	float processFrame(Params p) {
		m_iter += p.freq / p.srate;
		const float ph = hpposmodf(m_iter);
		const uint8_t iter = m_iter < 0 ? -m_iter : m_iter;
		// split the 8 bits of "iter" into 4 variables (A0,A1,B0,B1)
		const uint8_t  iterA  = iter & 0xf;
		uint8_t        iterA0 = iterA & 0x3;
		const uint8_t  iterA1 = iterA >> 2;
		const uint8_t  iterB  = (iter>>4) & 0xf;
		uint8_t        iterB0 = iterB & 0x3;
		const uint8_t  iterB1 = iterB >> 2;
		// mingle some bits
		iterA0 = ((iterA0 & 1) << 1) | (iterA0 >> 1);
		iterB0 = ((iterB0 & 1) << 1) | (iterB0 >> 1);
		// depending on the current quarter of the phase, output one of the 4
		// (this is construed so that the first wave is a "square")
		if (ph < 0.25f) {
			return 1 - iterA0 / 4.0f;
		}
		if (ph < 0.5f) {
			return 1 - iterA1 / 4.0f;
		}
		if (ph < 0.75f) {
			return -1 + iterB0 / 4.0f;
		}
		return -1 + iterB1 / 4.0f;
	}
};

inline unique_ptr<HPNode> instantiateNoiseDet(HPModel* model, int model_i) {
	return make_unique<HPNoiseDet>(
		model,
		model_i,
		static_pointer_cast<HPNoiseDetModel>(model->m_nodes[model_i])
	);
}

class HPNoiseDetView : public HPNodeView {
public:
	HPNoiseDetView(HPView* view) {}
	void setModel(weak_ptr<HPModel::Node> nmodel) {}
};

using Definition = HPDefinition<HPNoiseDetModel>;

template<> Definition::HPDefinition(HPInstrument* instrument) :
		HPDefinitionBase(instrument)
{
	m_forbidsArguments = true;
}

template<> Definition::~HPDefinition() = default;

template<> string Definition::name() { return NOISE_DET_NAME; }

template<> unique_ptr<HPNoiseDetModel> Definition::newNodeImpl() {
	return make_unique<HPNoiseDetModel>(m_instrument);
}

template<> unique_ptr<HPNodeView> Definition::instantiateView(HPView* hpview) {
	return make_unique<HPNoiseDetView>(hpview);
}

} // namespace lmms::hyperpipe
