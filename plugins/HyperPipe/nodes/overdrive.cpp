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
			m_drive(0.0f, -50.0f, 50.0f, 0.1f, instrument, QString("drive (+dB)")),
			m_makeup(0.0f, -50.0f, 50.0f, 0.1f, instrument, QString("makeup (-dB)"))
	{}
	FloatModel m_drive;
	FloatModel m_makeup;
	unique_ptr<HPNode> instantiate(HPModel* model, int model_i) {
		return instantiateOverdrive(model, model_i);
	}
	string name() { return OVERDRIVE_NAME; }
	void load(int model_i, const QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_drive.loadSettings(elem, is + "_drive");
		m_makeup.loadSettings(elem, is + "_makeup");
	}
	void save(int model_i, QDomDocument& doc, QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_drive.saveSettings(doc, elem, is + "_drive");
		m_makeup.saveSettings(doc, elem, is + "_makeup");
	}
};

class HPOverdrive : public HPNode
{
public:
	HPOverdrive(HPModel* model, int model_i, HPOverdriveModel* nmodel) :
			m_drive(&nmodel->m_drive),
			m_makeup(&nmodel->m_makeup),
			m_prev(model->instantiatePrev(model_i))
	{}
private:
	float processFrame(float freq, float srate) {
		if (m_prev == nullptr) {
			return 0.0f;
		}
		float prev = m_prev->processFrame(freq, srate);
		float drive = m_drive->value();
		drive = powf(10.0f, drive / 20);
		float makeup = m_makeup->value();
		makeup = powf(10.0f, -makeup / 20);
		return makeup * atanf(drive * prev);
	}
	FloatModel *m_drive;
	FloatModel *m_makeup;
	unique_ptr<HPNode> m_prev;
};

inline unique_ptr<HPNode> instantiateOverdrive(HPModel* model, int model_i) {
	return make_unique<HPOverdrive>(
		model,
		model_i,
		static_cast<HPOverdriveModel*>(model->m_nodes[model_i].get())
	);
}

class HPOverdriveView : public HPNodeView {
public:
	HPOverdriveView(HPView* view) :
			m_drive(new Knob(view, "drive (+dB)")),
			m_makeup(new Knob(view, "makeup (-dB)"))
	{
		m_widgets.emplace_back(m_drive);
		m_makeup->move(30, 0);
		m_widgets.emplace_back(m_makeup);
	}
	void setModel(HPModel::Node *model) {
		HPOverdriveModel *modelCast = static_cast<HPOverdriveModel*>(model);
		m_drive->setModel(&modelCast->m_drive);
		m_makeup->setModel(&modelCast->m_makeup);
	}
private:
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
