/*
	overdrive.cpp - implementation of the "overdrive" node type

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

const string OVERDRIVE_NAME = "overdrive";

inline unique_ptr<HPNode> instantiateOverdrive(HPModel* model, int model_i);

struct HPOverdriveModel : public HPModel::Node {
	HPOverdriveModel(Instrument* instrument) :
			Node(instrument),
			m_exp(1.0f, 0.05f, 20.0f, 0.05f, instrument, QString("od exp")),
			m_drive(0.0f, -50.0f, 50.0f, 0.1f, instrument, QString("drive (+dB)")),
			m_makeup(0.0f, -50.0f, 50.0f, 0.1f, instrument, QString("makeup (-dB)"))
	{}
	FloatModel m_exp;
	FloatModel m_drive;
	FloatModel m_makeup;
	unique_ptr<HPNode> instantiate(HPModel* model, int model_i) {
		return instantiateOverdrive(model, model_i);
	}
	string name() { return OVERDRIVE_NAME; }
	void load(int model_i, const QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_exp.loadSettings(elem, is + "_exp");
		m_drive.loadSettings(elem, is + "_drive");
		m_makeup.loadSettings(elem, is + "_makeup");
	}
	void save(int model_i, QDomDocument& doc, QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_exp.saveSettings(doc, elem, is + "_exp");
		m_drive.saveSettings(doc, elem, is + "_drive");
		m_makeup.saveSettings(doc, elem, is + "_makeup");
	}
	bool usesPrev() { return true; }
};

class HPOverdrive : public HPNode
{
public:
	HPOverdrive(HPModel* model, int model_i, shared_ptr<HPOverdriveModel> nmodel) :
			m_nmodel(nmodel),
			m_prev(model->instantiatePrev(model_i))
	{}
private:
	float processFrame(Params p) {
		if (m_prev == nullptr) {
			return 0.0f;
		}
		float drive = m_nmodel->m_drive.value();
		drive = powf(10.0f, drive / 20);
		float makeup = m_nmodel->m_makeup.value();
		makeup = powf(10.0f, -makeup / 20);
		float sample = m_prev->processFrame(p);
		// apply drive and atan
		sample = atanf(drive * sample);
		// apply exp
		float exp = m_nmodel->m_exp.value();
		if (sample >= 0) {
			sample = powf(sample, exp);
		}
		else {
			sample = -powf(-sample, exp);
		}
		// apply makeup
		sample = makeup * sample;
		// return result
		return sample;
	}
	void resetState() override {
		HPNode::resetState();
		if (m_prev != nullptr) {
			m_prev->resetState();
		}
	}
	shared_ptr<HPOverdriveModel> m_nmodel;
	unique_ptr<HPNode> m_prev;
};

inline unique_ptr<HPNode> instantiateOverdrive(HPModel* model, int model_i) {
	return make_unique<HPOverdrive>(
		model,
		model_i,
		static_pointer_cast<HPOverdriveModel>(model->m_nodes[model_i])
	);
}

class HPOverdriveView : public HPNodeView {
public:
	HPOverdriveView(HPView* view) :
			m_exp(new Knob(view, "od exp")),
			m_drive(new Knob(view, "drive (+dB)")),
			m_makeup(new Knob(view, "makeup (-dB)"))
	{
		m_widgets.emplace_back(m_exp);
		m_drive->move(30, 0);
		m_widgets.emplace_back(m_drive);
		m_makeup->move(60, 0);
		m_widgets.emplace_back(m_makeup);
	}
	void setModel(weak_ptr<HPModel::Node> nmodel) {
		auto modelCast = static_cast<HPOverdriveModel*>(nmodel.lock().get());
		m_exp->setModel(&modelCast->m_exp);
		m_drive->setModel(&modelCast->m_drive);
		m_makeup->setModel(&modelCast->m_makeup);
	}
private:
	Knob *m_exp;
	Knob *m_drive;
	Knob *m_makeup;
};

using Definition = HPDefinition<HPOverdriveModel>;

template<> Definition::HPDefinition(HPInstrument* instrument) :
		HPDefinitionBase(instrument)
{
	m_forbidsArguments = true;
}

template<> Definition::~HPDefinition() = default;

template<> string Definition::name() { return OVERDRIVE_NAME; }

template<> unique_ptr<HPOverdriveModel> Definition::newNodeImpl() {
	return make_unique<HPOverdriveModel>(m_instrument);
}

template<> unique_ptr<HPNodeView> Definition::instantiateView(HPView* hpview) {
	return make_unique<HPOverdriveView>(hpview);
}

} // namespace lmms::hyperpipe
