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

struct HPShapesModel : public HPModel::Node {
	HPShapesModel(Instrument* instrument) :
			Node(instrument),
			m_shape(make_shared<FloatModel>(0.0f, -3.0f, 3.0f, 0.01f, instrument, QString("shape"))),
			m_jitter(make_shared<FloatModel>(0.0f, 0.0f, 100.0f, 1.0f, instrument, QString("jitter"))),
			m_smoothstep(make_shared<BoolModel>(false, instrument, QString("shapes smoothstep")))
	{}
	shared_ptr<FloatModel> m_shape;
	shared_ptr<FloatModel> m_jitter;
	shared_ptr<BoolModel> m_smoothstep;
	unique_ptr<HPNode> instantiate(HPModel* model, int model_i) {
		return instantiateShapes(model, model_i);
	}
	string name() {
		return SHAPES_NAME;
	}
	void load(int model_i, const QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_shape->loadSettings(elem, is + "_shape");
		m_jitter->loadSettings(elem, is + "_jitter");
		m_smoothstep->loadSettings(elem, is + "_smoothstep");
	}
	void save(int model_i, QDomDocument& doc, QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_shape->saveSettings(doc, elem, is + "_shape");
		m_jitter->saveSettings(doc, elem, is + "_jitter");
		m_smoothstep->saveSettings(doc, elem, is + "_smoothstep");
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
			HPOsc(model, model_i),
			m_shape(nmodel->m_shape),
			m_jitter(nmodel->m_jitter),
			m_smoothstep(nmodel->m_smoothstep)
	{}
private:
	float shape(float ph) {
		//continuously add to the jittering
		m_jitterState = (127.0f * m_jitterState + fastRandf(m_jitter->value())) / 128.0f;
		float shape = m_shape->value() + m_jitterState - 0.5f * m_jitter->value();
		shape = hpposmodf(shape, 3.0f);
		uint8_t shapeType = uint8_t(shape);
		float morph = fraction(shape);
		morph = hpsstep(morph);
		float amp; //the shapes with vertical edges sound too loud
		float s;
		if (shapeType == 0) {
			amp = 0.4f + 0.6f * morph;
			s = saw2tri(ph, morph);
		}
		else if (shapeType == 1) {
			amp = 1.0f - 0.7f * morph;
			s = tri2sqr(ph, morph);
		}
		else { // shapeType == 2
			amp = 0.3f + 0.1f * morph;
			s = sqr2saw(ph, morph);
		}
		if (m_smoothstep->value()) {
			return amp * 2 * hpsstep((s + 1) / 2) - 1;
		}
		return amp * s;
	}
	shared_ptr<FloatModel> m_shape;
	shared_ptr<FloatModel> m_jitter;
	shared_ptr<BoolModel> m_smoothstep;
	float m_jitterState = 0.0f;
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
			m_shape(view, "shape"),
			m_jitter(view, "jitter"),
			m_smoothstep(view, "shapes smoothstep")
	{
		m_widgets.emplace_back(&m_shape);
		m_widgets.emplace_back(&m_jitter);
		m_jitter.move(40, 0);
		m_widgets.emplace_back(&m_smoothstep);
		m_smoothstep.move(80, 0);
	}
	void setModel(shared_ptr<HPModel::Node> model) {
		shared_ptr<HPShapesModel> modelCast = static_pointer_cast<HPShapesModel>(model);
		m_shape.setModel(modelCast->m_shape.get());
		m_jitter.setModel(modelCast->m_jitter.get());
		m_smoothstep.setModel(modelCast->m_smoothstep.get());
	}
private:
	Knob m_shape;
	Knob m_jitter;
	LedCheckBox m_smoothstep;
};

using Definition = HPDefinition<HPShapesModel>;

template<> Definition::HPDefinition(HPInstrument* instrument) :
		HPDefinitionBase(instrument)
{}

template<> Definition::~HPDefinition() = default;

template<> string Definition::name() { return SHAPES_NAME; }

template<> shared_ptr<HPShapesModel> Definition::newNodeImpl() {
	return make_shared<HPShapesModel>(m_instrument);
}

template<> unique_ptr<HPNodeView> Definition::instantiateView(HPView* hpview) {
	return make_unique<HPShapesView>(hpview);
}


} // namespace lmms::hyperpipe
