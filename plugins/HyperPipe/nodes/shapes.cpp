/*
	shapes.cpp - implementation of the "shapes" node type

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

const string SHAPES_NAME = "shapes";
const string HPDefinitionBase::DEFAULT_TYPE = SHAPES_NAME;

inline unique_ptr<HPNode> instantiateShapes(HPModel* model, int model_i);

struct HPShapesModel : public HPOscModel {
	HPShapesModel(Instrument* instrument) :
			HPOscModel(instrument),
			m_shape(0.0f, -3.0f, 3.0f, 0.01f, instrument, QString("shape")),
			m_smoothstep(false, instrument, QString("shapes smoothstep")),
			m_corr(true, instrument, QString("shapes amp correction"))
	{}
	FloatModel m_shape;
	BoolModel m_smoothstep;
	BoolModel m_corr;
	unique_ptr<HPNode> instantiate(HPModel* model, int model_i) {
		return instantiateShapes(model, model_i);
	}
	string name() { return SHAPES_NAME; }
	void loadImpl(int model_i, const QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_shape.loadSettings(elem, is + "_shape");
		m_smoothstep.loadSettings(elem, is + "_smoothstep");
		m_corr.loadSettings(elem, is + "_corr");
	}
	void saveImpl(int model_i, QDomDocument& doc, QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_shape.saveSettings(doc, elem, is + "_shape");
		m_smoothstep.saveSettings(doc, elem, is + "_smoothstep");
		m_corr.saveSettings(doc, elem, is + "_corr");
	}
};

inline float saw2tri(float ph, float morph)
{
	// left and right edge of each segment
	float le, re;
	le = 0.0f;
	re = morph * 0.25f;
	if (ph < re) {
		//0.0...1.0
		return ph / re;
	}
	le = re;
	re = 1.0f - morph * 0.25f;
	if (ph < re) {
		//this is the main (saw) shape
		//1.0...-1.0
		return 1.0f - 2.0f * (ph - le) / (re - le);
	}
	le = re;
	re = 1.0f;
	//-1.0...0.0
	return -1.0f + (ph - le) / (re - le);
}

inline float sqr2saw(float ph, float morph)
{
	// left and right edge of each segment
	float le, re;
	le = 0.0f;
	re = 0.5f - morph * 0.5f;
	if (ph < re) {
		return 1.0f;
	}
	le = re;
	re = 0.5f + morph * 0.5f;
	if (ph < re) {
		return 1.0f - 2.0f * (ph - le) / (re - le);
	}
	//re = 1.0f;
	return -1.0f;
}

inline float tri2sqr(float ph, float morph)
{
	// left and right edge of each segment
	float le, re;
	le = 0.0;
	re = 0.25f - morph * 0.25f;
	if (ph < re) {
		return ph / re;
	}
	//le = re;
	re = 0.25f + morph * 0.25f;
	if (ph < re) {
		return 1.0f;
	}
	le = re;
	re = 0.75f - morph * 0.25f;
	if (ph < re) {
		return 1.0f - 2.0f * (ph - le) / (re - le);
	}
	le = re;
	re = 0.75f + morph * 0.25f;
	if (ph < re) {
		return -1.0f;
	}
	le = re;
	re = 1.0f;
	return -1.0f + (ph - le) / (re - le);
}

class HPShapes : public HPOsc
{
public:
	HPShapes(HPModel* model, int model_i, shared_ptr<HPShapesModel> nmodel) :
			HPOsc(model, model_i, nmodel),
			m_nmodel(nmodel)
	{}
private:
	float shape(float ph) {
		float shape = m_nmodel->m_shape.value();
		shape = hpposmodf(shape, 3.0f);
		uint8_t shapeType = uint8_t(shape);
		float morph = hpposmodf(shape);
		morph = hpsstep(morph);
		float amp = 1; //the shapes with vertical edges sound too loud
		bool corr = m_nmodel->m_corr.value();
		float s;
		if (shapeType == 0) {
			if (corr) {
				amp = 0.4f + 0.6f * morph;
			}
			s = saw2tri(ph, morph);
		}
		else if (shapeType == 1) {
			if (corr) {
				amp = 1.0f - 0.7f * morph;
			}
			s = tri2sqr(ph, morph);
		}
		else { // shapeType == 2
			if (corr) {
				amp = 0.3f + 0.1f * morph;
			}
			s = sqr2saw(ph, morph);
		}
		if (corr) {
			amp = hpsstep(amp); // morph is smooth, so amp is, too
		}
		if (! m_nmodel->m_smoothstep.value()) {
			return amp * s;
		}
		// with smoothstep:
		s = (s + 1) / 2; // 0.0...1.0
		s = hpsstep(s);
		s = -1 + 2 * s; // -1.0...1.0
		return amp * s;
	}
	shared_ptr<HPShapesModel> m_nmodel;
};

inline unique_ptr<HPNode> instantiateShapes(HPModel* model, int model_i) {
	return make_unique<HPShapes>(
		model,
		model_i,
		static_pointer_cast<HPShapesModel>(model->m_nodes[model_i])
	);
}

class HPShapesView : public HPNodeView {
public:
	HPShapesView(HPView* view) :
			m_shape(new Knob(view, "shape")),
			m_smoothstep(new LedCheckBox(view, "shapes smoothstep")),
			m_corr(new LedCheckBox(view, "shapes amp correction"))
	{
		m_widgets.emplace_back(m_shape);
		m_widgets.emplace_back(m_smoothstep);
		m_smoothstep->move(30, 0);
		m_widgets.emplace_back(m_corr);
		m_corr->move(50, 0);
	}
	void setModel(weak_ptr<HPModel::Node> nmodel) {
		auto modelCast = static_cast<HPShapesModel*>(nmodel.lock().get());
		m_shape->setModel(&modelCast->m_shape);
		m_smoothstep->setModel(&modelCast->m_smoothstep);
		m_corr->setModel(&modelCast->m_corr);
	}
private:
	Knob *m_shape;
	LedCheckBox *m_smoothstep;
	LedCheckBox *m_corr;
};

using Definition = HPDefinition<HPShapesModel>;

template<> Definition::HPDefinition(HPInstrument* instrument) :
		HPDefinitionBase(instrument)
{}

template<> Definition::~HPDefinition() = default;

template<> string Definition::name() { return SHAPES_NAME; }

template<> unique_ptr<HPShapesModel> Definition::newNodeImpl() {
	return make_unique<HPShapesModel>(m_instrument);
}

template<> unique_ptr<HPNodeView> Definition::instantiateView(HPView* hpview) {
	return make_unique<HPShapesView>(hpview);
}


} // namespace lmms::hyperpipe
