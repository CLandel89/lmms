/*
	sine.cpp - implementation of the "sine" node type

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

const string SINE_NAME = "sine";

inline unique_ptr<HPNode> instantiateSine(HPModel* model, int model_i);

struct HPSineModel : public HPOscModel {
	HPSineModel(Instrument* instrument) :
			HPOscModel(instrument)
	{}
	unique_ptr<HPNode> instantiate(HPModel* model, int model_i) {
		return instantiateSine(model, model_i);
	}
	void loadImpl(int model_i, const QDomElement& elem) {}
	void saveImpl(int model_i, QDomDocument& doc, QDomElement& elem) {}
	string name() { return SINE_NAME; }
};

class HPSine : public HPOsc
{
public:
	HPSine(HPModel* model, int model_i, shared_ptr<HPSineModel> nmodel) :
			HPOsc(model, model_i, nmodel)
	{}
private:
	float shape(float ph) {
		return sinf(F_2PI * ph);
	}
};

inline unique_ptr<HPNode> instantiateSine(HPModel* model, int model_i) {
	return make_unique<HPSine>(
		model,
		model_i,
		static_pointer_cast<HPSineModel>(model->m_nodes[model_i])
	);
}

class HPSineView : public HPNodeView {
public:
	HPSineView(HPView* view) {}
	void setModel(weak_ptr<HPModel::Node> nmodel) {}
};

using Definition = HPDefinition<HPSineModel>;

template<> Definition::HPDefinition(HPInstrument* instrument) :
		HPDefinitionBase(instrument)
{}

template<> Definition::~HPDefinition() = default;

template<> string Definition::name() { return SINE_NAME; }

template<> unique_ptr<HPSineModel> Definition::newNodeImpl() {
	return make_unique<HPSineModel>(m_instrument);
}

template<> unique_ptr<HPNodeView> Definition::instantiateView(HPView* hpview) {
	return make_unique<HPSineView>(hpview);
}

} // namespace lmms::hyperpipe