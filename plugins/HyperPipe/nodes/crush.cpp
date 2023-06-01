/*
	reverb.cpp - implementation of the "reverb" node type, a buffered reverb

	HyperPipe - synth with arbitrary possibilities

	Copyright (c) 2023 Christian Landel

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

const string CRUSH_NAME = "crush";

inline unique_ptr<HPNode> instantiateCrush(HPModel* model, int model_i);

struct HPCrushModel : public HPModel::Node {
	HPCrushModel(Instrument* instrument) :
			Node(instrument),
			m_i(0.00f, 0.00f, 50.0f, 0.01f, instrument, QString("crush intensity"))
	{}
	FloatModel m_i;
	unique_ptr<HPNode> instantiate(HPModel* model, int model_i) {
		return instantiateCrush(model, model_i);
	}
	string name() { return CRUSH_NAME; }
	void load(int model_i, const QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_i.loadSettings(elem, is + "_i");
	}
	void save(int model_i, QDomDocument& doc, QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_i.saveSettings(doc, elem, is + "_i");
	}
	bool usesPrev() { return true; }
};

class HPCrush : public HPNode
{
public:
	HPCrush(HPModel* model, int model_i, shared_ptr<HPCrushModel> nmodel) :
			m_nmodel(nmodel),
			m_prev(model->instantiatePrev(model_i))
	{}
private:
	float processFrame(Params p)
	{
		if (m_prev == nullptr) {
			return 0;
		}
		float prev = m_prev->processFrame(p);
		float i = m_nmodel->m_i.value();
		return (prev + sinf(i * prev * 2 * M_PI)) / 2;
	}
	void resetState() override {
		HPNode::resetState();
		if (m_prev != nullptr) {
			m_prev->resetState();
		}
	}
	shared_ptr<HPCrushModel> m_nmodel;
	unique_ptr<HPNode> m_prev = nullptr;
};

inline unique_ptr<HPNode> instantiateCrush(HPModel* model, int model_i) {
	return make_unique<HPCrush>(
		model,
		model_i,
		static_pointer_cast<HPCrushModel>(model->m_nodes[model_i])
	);
}

class HPCrushView : public HPNodeView {
public:
	HPCrushView(HPView* view) :
			m_i(new Knob(view, "crush intensity"))
	{
		m_widgets.emplace_back(m_i);
	}
	void setModel(weak_ptr<HPModel::Node> nmodel) {
		auto modelCast = static_cast<HPCrushModel*>(nmodel.lock().get());
		m_i->setModel(&modelCast->m_i);
	}
private:
	Knob *m_i;
};

using Definition = HPDefinition<HPCrushModel>;

template<> Definition::HPDefinition(HPInstrument* instrument) :
		HPDefinitionBase(instrument)
{
		m_forbidsArguments = true;
}

template<> Definition::~HPDefinition() = default;

template<> string Definition::name() { return CRUSH_NAME; }

template<> unique_ptr<HPCrushModel> Definition::newNodeImpl() {
	return make_unique<HPCrushModel>(m_instrument);
}

template<> unique_ptr<HPNodeView> Definition::instantiateView(HPView* hpview) {
	return make_unique<HPCrushView>(hpview);
}

} // namespace lmms::hyperpipe
