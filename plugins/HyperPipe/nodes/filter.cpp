/*
	filter.cpp - implementation of the "filter" node type (uses lmms::BasicFilters)

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

#include "BasicFilters.h"

namespace lmms::hyperpipe
{

const string FILTER_NAME = "filter";

inline unique_ptr<HPNode> instantiateFilter(HPModel* model, int model_i);

struct HPFilterModel : public HPModel::Node {
	HPFilterModel(Instrument* instrument) :
			Node(instrument),
			m_reso(0.5f, BasicFilters<>::minQ(), 10.0f, 0.01f, instrument, QString("Q/Resonance")),
			m_amp(10.0f, 0.0f, 40.0f, 0.1f, instrument, QString("filter makeup"))
	{}
	FloatModel m_reso;
	FloatModel m_amp;
	unique_ptr<HPNode> instantiate(HPModel* model, int model_i) {
		return instantiateFilter(model, model_i);
	}
	string name() { return FILTER_NAME; }
	void load(int model_i, const QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_reso.loadSettings(elem, is + "_reso");
		m_amp.loadSettings(elem, is + "_amp");
	}
	void save(int model_i, QDomDocument& doc, QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_reso.saveSettings(doc, elem, is + "_reso");
		m_amp.saveSettings(doc, elem, is + "_amp");
	}
	bool usesPrev() { return true; }
};

class HPFilter : public HPNode
{
public:
	HPFilter(HPModel* model, int model_i, shared_ptr<HPFilterModel> nmodel) :
			m_nmodel(nmodel),
			m_prev(model->instantiatePrev(model_i)),
			m_srate_tmp(Engine::audioEngine()->outputSampleRate()),
			m_basicFilters(m_srate_tmp)
	{
		m_basicFilters.setFilterType(m_filterType);
	}
private:
	float processFrame(Params p) {
		if (m_prev == nullptr) {
			return 0.0f;
		}
		if (m_srate_tmp != p.srate) {
			m_basicFilters = BasicFilters<1>(p.srate);
			m_basicFilters.setFilterType(m_filterType);
			m_srate_tmp = p.srate;
		}
		float reso = m_nmodel->m_reso.value();
		if (m_freqReso_tmp != make_tuple(p.freq, reso)) {
			m_basicFilters.calcFilterCoeffs(p.freq, reso);
			m_freqReso_tmp = make_tuple(p.freq, reso);
		}
		float f = m_prev->processFrame(p);
		float a = m_nmodel->m_amp.value();
		a = powf(10.0f, a / 20);
		return a * m_basicFilters.update(f, 0);
	}
	void resetState() override {
		HPNode::resetState();
		if (m_prev != nullptr) {
			m_prev->resetState();
		}
		// provoke a state reset
		m_srate_tmp = -1;
	}
	shared_ptr<HPFilterModel> m_nmodel;
	unique_ptr<HPNode> m_prev = nullptr;
	float m_srate_tmp;
	tuple<float, float> m_freqReso_tmp = make_tuple(-1, -1);
	BasicFilters<1> m_basicFilters;
	static const auto m_filterType = BasicFilters<1>::FilterType::Bandpass_RC24;
};

inline unique_ptr<HPNode> instantiateFilter(HPModel* model, int model_i) {
	return make_unique<HPFilter>(
		model,
		model_i,
		static_pointer_cast<HPFilterModel>(model->m_nodes[model_i])
	);
}

class HPFilterView : public HPNodeView {
public:
	HPFilterView(HPView* view) :
			m_reso(new Knob(view, "Q/Resonance")),
			m_amp(new Knob(view, "filter makeup"))
	{
		m_widgets.emplace_back(m_reso);
		m_widgets.emplace_back(m_amp);
		m_amp->move(30, 0);
	}
	void setModel(weak_ptr<HPModel::Node> nmodel) {
		auto modelCast = static_cast<HPFilterModel*>(nmodel.lock().get());
		m_reso->setModel(&modelCast->m_reso);
		m_amp->setModel(&modelCast->m_amp);
	}
private:
	Knob *m_reso;
	Knob *m_amp;
};

using Definition = HPDefinition<HPFilterModel>;

template<> Definition::HPDefinition(HPInstrument* instrument) :
		HPDefinitionBase(instrument)
{
		m_forbidsArguments = true;
}

template<> Definition::~HPDefinition() = default;

template<> string Definition::name() { return FILTER_NAME; }

template<> unique_ptr<HPFilterModel> Definition::newNodeImpl() {
	return make_unique<HPFilterModel>(m_instrument);
}

template<> unique_ptr<HPNodeView> Definition::instantiateView(HPView* hpview) {
	return make_unique<HPFilterView>(hpview);
}

} // namespace lmms::hyperpipe
