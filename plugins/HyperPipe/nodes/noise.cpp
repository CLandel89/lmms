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

inline unique_ptr<HPNode> instantiateNoise();

struct HPNoiseModel : public HPModel::Node {
	HPNoiseModel(Instrument* instrument) :
			HPModel::Node(instrument)
	{}
	unique_ptr<HPNode> instantiate(HPModel* model, int model_i) {
		return instantiateNoise();
	}
	string name() { return NOISE_NAME; }
	void load(int model_i, const QDomElement& elem) {}
	void save(int model_i, QDomDocument& doc, QDomElement& elem) {}
	bool usesPrev() { return false; }
};

class HPNoise : public HPNode
{
	float m_excess = 0;
public:
	float processFrame(Params p) {
		float f = 1 - fastRandf(2) - m_excess / 4;
		if (f > 1) {
			f = 1;
		}
		if (f < -1) {
			f = -1;
		}
		m_excess += f;
		return f;
	}
};

inline unique_ptr<HPNode> instantiateNoise() { return make_unique<HPNoise>(); }

class HPNoiseView : public HPNodeView {
public:
	HPNoiseView(HPView* view, HPInstrument* instrument) {}
	void setModel(weak_ptr<HPModel::Node> model) {}
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
