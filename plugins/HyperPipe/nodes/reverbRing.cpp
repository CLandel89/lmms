/*
	reverb.cpp - implementation of the "reverb" node type, a buffered reverb

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

#include <cmath>

#include "../HyperPipe.h"

namespace lmms::hyperpipe
{

const string REVERB_RING_NAME = "reverb (ring)";

inline unique_ptr<HPNode> instantiateReverbRing(HPModel* model, int model_i);

struct HPReverbRingModel : public HPModel::Node {
	HPReverbRingModel(Instrument* instrument) :
			Node(instrument),
			m_wd(1.00f, 0.00f, 1.0f, 0.01f, instrument, QString("reverb w/d")),
			m_clipDb(0.0f, -60.0f, 60.0f, 0.01f, instrument, QString("reverb clip at x dB")),
			m_fback(0.0f, 0.0f, 12.5f, 0.1f, instrument, QString("reverb feedback (%)")),
			m_factor(0.03f, 0.0f, 0.99f, 0.01f, instrument, QString("reverb factor")),
			m_db(5.0f, -60.0f, 60.0f, 0.1f, instrument, QString("reverb +dB"))
	{}
	FloatModel m_wd;
	FloatModel m_clipDb;
	FloatModel m_fback;
	FloatModel m_factor;
	FloatModel m_db;
	unique_ptr<HPNode> instantiate(HPModel* model, int model_i) {
		return instantiateReverbRing(model, model_i);
	}
	string name() { return REVERB_RING_NAME; }
	void load(int model_i, const QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_wd.loadSettings(elem, is + "_wd");
		m_clipDb.loadSettings(elem, is + "_clipDb");
		m_fback.loadSettings(elem, is + "_fback");
		m_factor.loadSettings(elem, is + "_factor");
		m_db.loadSettings(elem, is + "_db");
	}
	void save(int model_i, QDomDocument& doc, QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_wd.saveSettings(doc, elem, is + "_wd");
		m_clipDb.saveSettings(doc, elem, is + "_clipDb");
		m_fback.saveSettings(doc, elem, is + "_fback");
		m_factor.saveSettings(doc, elem, is + "_factor");
		m_db.saveSettings(doc, elem, is + "_db");
	}
	bool usesPrev() { return true; }
};

class HPReverbRing : public HPNode
{
public:
	HPReverbRing(HPModel* model, int model_i, shared_ptr<HPReverbRingModel> nmodel) :
			m_nmodel(nmodel),
			m_prev(model->instantiatePrev(model_i))
	{
		m_buf = vector<float>();
		m_buf.push_back(0.0f);
	}
private:
	float processFrame(Params p)
	{
		_rescaleIfNecessairy(p);
		// get dry part from previous node
		float dry;
		if (m_prev == nullptr) {
			dry = 0.0f;
		}
		else {
			dry = m_prev->processFrame(p);
		}
		// get wet part from ring buffer
		float wet = m_buf[m_buf_i];
		// write back to ring buffer
		float fback = m_nmodel->m_fback.value();
		float factor = m_nmodel->m_factor.value();
		m_buf[m_buf_i] *= 1-factor + fback/100;
		m_buf[m_buf_i] += factor * dry;
		// update ring buffer index
		m_buf_i++;
		if (m_buf_i == m_buf.size()) {
			m_buf_i = 0;
			// skip / repeat samples to reduce aliasing effects
			if (m_skipCountdown-- == 0) {
				m_skipCountdown = m_skipPass++;
				return processFrame(p);
			}
		}
		// calculate result
		float wd = m_nmodel->m_wd.value();
		float db = m_nmodel->m_db.value();
		float a = powf(10, db / 20);
		float unclipped = wd * a * wet + (1 - wd) * dry;
		float clipDb = m_nmodel->m_clipDb.value();
		float maxAmp = powf(10, clipDb / 20);
		if (unclipped > maxAmp) {
			return maxAmp;
		}
		if (unclipped < -maxAmp) {
			return -maxAmp;
		}
		return unclipped;
	}
	void _rescaleIfNecessairy(Params p) {
		int newSize = p.srate / p.freq;
		int oldSize = m_buf.size();
		if (oldSize == newSize) {
			return;
		}
		// nearest-hit strategy
		vector<float> newBuf(newSize);
		for (int newPos = 0; newPos < newSize; newPos++) {
			int oldPos = round(newPos * oldSize / newSize);
			int old_i = (oldPos + m_buf_i) % oldSize;
			newBuf[newPos] = m_buf[old_i];
		}
		m_buf = newBuf;
		m_buf_i = 0;
	}
	void resetState() override {
		HPNode::resetState();
		if (m_prev != nullptr) {
			m_prev->resetState();
		}
		m_buf = vector<float>(m_buf.size());
	}
	shared_ptr<HPReverbRingModel> m_nmodel;
	unique_ptr<HPNode> m_prev = nullptr;
	vector<float> m_buf;
	size_t m_buf_i = 0;
	int m_skipCountdown = 1;
	int m_skipPass = 2;
};

inline unique_ptr<HPNode> instantiateReverbRing(HPModel* model, int model_i) {
	return make_unique<HPReverbRing>(
		model,
		model_i,
		static_pointer_cast<HPReverbRingModel>(model->m_nodes[model_i])
	);
}

class HPReverbRingView : public HPNodeView {
public:
	HPReverbRingView(HPView* view) :
			m_wd(new Knob(view, "reverb w/d")),
			m_clipDb(new Knob(view, "reverb clip at x dB")),
			m_fback(new Knob(view, "reverb feedback (%)")),
			m_factor(new Knob(view, "reverb factor")),
			m_db(new Knob(view, "reverb +dB"))
	{
		m_widgets.emplace_back(m_wd);
		m_widgets.emplace_back(m_clipDb);
		m_clipDb->move(30, 0);
		m_widgets.emplace_back(m_fback);
		m_fback->move(60, 0);
		m_widgets.emplace_back(m_factor);
		m_factor->move(90, 0);
		m_widgets.emplace_back(m_db);
		m_db->move(120, 0);
	}
	void setModel(weak_ptr<HPModel::Node> nmodel) {
		auto modelCast = static_cast<HPReverbRingModel*>(nmodel.lock().get());
		m_wd->setModel(&modelCast->m_wd);
		m_clipDb->setModel(&modelCast->m_clipDb);
		m_fback->setModel(&modelCast->m_fback);
		m_factor->setModel(&modelCast->m_factor);
		m_db->setModel(&modelCast->m_db);
	}
private:
	Knob *m_wd;
	Knob *m_clipDb;
	Knob *m_fback;
	Knob *m_factor;
	Knob *m_db;
};

using Definition = HPDefinition<HPReverbRingModel>;

template<> Definition::HPDefinition(HPInstrument* instrument) :
		HPDefinitionBase(instrument)
{
		m_forbidsArguments = true;
}

template<> Definition::~HPDefinition() = default;

template<> string Definition::name() { return REVERB_RING_NAME; }

template<> unique_ptr<HPReverbRingModel> Definition::newNodeImpl() {
	return make_unique<HPReverbRingModel>(m_instrument);
}

template<> unique_ptr<HPNodeView> Definition::instantiateView(HPView* hpview) {
	return make_unique<HPReverbRingView>(hpview);
}

} // namespace lmms::hyperpipe
