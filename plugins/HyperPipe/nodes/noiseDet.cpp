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
			HPModel::Node(instrument),
			m_mingle(
				57,
				1, 0xffff, instrument, QString("mingle factor"))
	{}
	IntModel m_mingle;
	unique_ptr<HPNode> instantiate(HPModel* model, int model_i) {
		return instantiateNoiseDet(model, model_i);
	}
	string name() { return NOISE_DET_NAME; }
	void load(int model_i, const QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_mingle.loadSettings(elem, is + "_mingle");
	}
	void save(int model_i, QDomDocument& doc, QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_mingle.saveSettings(doc, elem, is + "_mingle");
	}
	bool usesPrev() { return false; }
};

class HPNoiseDet : public HPNode
{
	int32_t m_iter = 0;  // if ph and iter aren't separate, audible glitches will occur
	float m_ph = 0;
public:
	HPNoiseDet(HPModel* model, int model_i, shared_ptr<HPNoiseDetModel> nmodel) :
			HPNode(),
			m_nmodel(nmodel)
	{}
	float processFrame(Params p) {
		uint16_t iter         = m_iter < 0 ? -m_iter : m_iter;
		iter *= m_nmodel->m_mingle.value();
		uint16_t gray = iter ^ (iter >> 1);  // https://de.wikipedia.org/wiki/Gray-Code#Generierung
		// mingle, bitwise
		if (iter & 1) {
			iter = 0;
			for (uint8_t b = 0; b < 8; b++) {
				uint16_t bit = (gray >> b) & 1;
				if (b & 1) {
					bit = (~bit) & 1;
				}
				iter |= bit << b;
			}
		}
		else {
			iter = 0;
			for (uint8_t b = 0; b < 8; b++) {
				uint16_t bit = (gray >> b) & 1;
				if (!(b & 1)) {
					bit = (~bit) & 1;
				}
				iter |= bit << b;
			}
		}
		// determine which bits to output
		const float hph = hpposmodf(2*p.ph);
		uint8_t step = 4 * hpposmodf(hph);
		if (p.ph >= 0.5f) {
			// the second half is backwards and mirrored
			step = 3 - step;
		}
		uint8_t output = (iter >> (step*4)) & 0xf;
		// https://de.wikipedia.org/wiki/Gray-Code#Gray-Code_zur%C3%BCckrechnen
		uint8_t mask = output;
		while (mask) {
			mask >>= 1;
			output ^= mask;
		}
		// inner state, output
		float outputf;
		if (p.ph >= 0.5f) {
			outputf = -1 + 2.0f * output / 0xf;
		}
		else {
			outputf = 1 - 2.0f * output / 0xf;
		}
		m_ph += p.freqMod / p.srate;
		m_iter += floor(m_ph);
		m_ph = hpposmodf(m_ph);
		return outputf;
	}
	shared_ptr<HPNoiseDetModel> m_nmodel;
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
	HPNoiseDetView(HPView* view) :
			m_mingle(new LcdSpinBox(5, view, "mingle factor"))
	{
		m_widgets.emplace_back(m_mingle);
	}
	void setModel(weak_ptr<HPModel::Node> nmodel) {
		auto modelCast = static_cast<HPNoiseDetModel*>(nmodel.lock().get());
		m_mingle->setModel(&modelCast->m_mingle);
	}
private:
	LcdSpinBox *m_mingle;
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
