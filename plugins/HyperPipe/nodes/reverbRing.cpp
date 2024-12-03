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
			m_factor(0.15f, 0.0f, 0.99f, 0.01f, instrument, QString("reverb factor")),
			m_passes(5, 1, 99, instrument, QString("reverb passes")),
			m_db(5.0f, -60.0f, 60.0f, 0.1f, instrument, QString("reverb +dB"))
	{}
	FloatModel m_wd;
	FloatModel m_clipDb;
	FloatModel m_fback;
	FloatModel m_factor;
	IntModel m_passes;
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
		m_passes.loadSettings(elem, is + "_passes");
		m_db.loadSettings(elem, is + "_db");
	}
	void save(int model_i, QDomDocument& doc, QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_wd.saveSettings(doc, elem, is + "_wd");
		m_clipDb.saveSettings(doc, elem, is + "_clipDb");
		m_fback.saveSettings(doc, elem, is + "_fback");
		m_factor.saveSettings(doc, elem, is + "_factor");
		m_passes.saveSettings(doc, elem, is + "_passes");
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
		_init();
	}
private:
	shared_ptr<HPReverbRingModel> m_nmodel;
	unique_ptr<HPNode> m_prev = nullptr;
	vector<vector<float>> m_buf;
	size_t m_buf_i = 0;
	int m_skipCountdown = 1;
	int m_skipSpacing = 2;

	float processFrame(Params p)
	{
		_adaptToPassesIfNeeded();
		_resizeIfNeeded(p);

		float dry, frame;
		if (m_prev == nullptr) {
			dry = frame = 0.0f;
		}
		else {
			dry = frame = m_prev->processFrame(p);
		}

		int passes = m_buf.size();
		for (int pass = 0; pass < passes; pass++) {
			frame = _processPass(p, frame, pass);
		}
		float wet = frame;

		m_buf_i++;
		if (m_buf_i == m_buf[0].size()) {
			m_buf_i = 0;
			// skip / repeat samples to reduce aliasing effects
			// (this can be tested on and around D5)
			if (m_skipCountdown-- == 0) {
				m_skipCountdown = m_skipSpacing++;
				return processFrame(p);
			}
		}

		float wd = m_nmodel->m_wd.value();
		float db = m_nmodel->m_db.value();
		float a = powf(10, db / 20);
		return wd * a * wet + (1 - wd) * dry;
	}

	void _init () {
		// dummy buffer content that gets replaced soon
		int passes = m_nmodel->m_passes.value();
		m_buf = vector<vector<float>>(passes);
		for (int pass = 0; pass < passes; pass++) {
			m_buf[pass].emplace_back(0.0f);
		}
	}

	float _processPass (Params p, float dry, int pass)
	{
		float clipDb = m_nmodel->m_clipDb.value();
		float fback = m_nmodel->m_fback.value();
		float factor = m_nmodel->m_factor.value();

		m_buf[pass][m_buf_i] *= 1-factor + fback/100;
		m_buf[pass][m_buf_i] += factor * dry;

		float maxAmp = powf(10, clipDb / 20);
		if (m_buf[pass][m_buf_i] > maxAmp) {
			m_buf[pass][m_buf_i] = maxAmp;
		}
		if (m_buf[pass][m_buf_i] < -maxAmp) {
			m_buf[pass][m_buf_i] = -maxAmp;
		}

		return m_buf[pass][m_buf_i];
	}

	void _adaptToPassesIfNeeded() {
		int newPasses = m_nmodel->m_passes.value();
		int oldPasses = m_buf.size();
		if (newPasses == oldPasses) {
			return;
		}
		// extend if needed
		vector<float> &lastRing = m_buf[oldPasses-1];
		for (int extraPass = oldPasses; extraPass < newPasses; extraPass++) {
			m_buf.emplace_back(lastRing);
		}
		// cut off if needed
		m_buf.resize(newPasses);
	}

	void _resizeIfNeeded(Params p) {
		int newSize = p.srate / p.freq;
		int oldSize = m_buf[0].size();
		if (oldSize == newSize) {
			return;
		}
		// nearest-hit strategy
		int passes = m_buf.size();
		vector<vector<float>> newBuf(passes);
		for (int pass = 0; pass < passes; pass++) {
			newBuf[pass] = vector<float>(newSize);
			for (int newPos = 0; newPos < newSize; newPos++) {
				int oldPos = round(float(newPos) * oldSize / newSize);
				int old_i = (oldPos + m_buf_i) % oldSize;
				newBuf[pass][newPos] = m_buf[pass][old_i];
			}
		}
		m_buf = newBuf;
		m_buf_i = 0;
	}

	void resetState() override {
		HPNode::resetState();
		if (m_prev != nullptr) {
			m_prev->resetState();
		}
		_init();
	}
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
			m_passes(new LcdSpinBox(2, view, "reverb passes")),
			m_db(new Knob(view, "reverb +dB"))
	{
		m_widgets.emplace_back(m_wd);
		m_widgets.emplace_back(m_clipDb);
		m_clipDb->move(30, 0);
		m_widgets.emplace_back(m_fback);
		m_fback->move(60, 0);
		m_widgets.emplace_back(m_factor);
		m_factor->move(90, 0);
		m_widgets.emplace_back(m_passes);
		m_passes->move(120, 0);
		m_widgets.emplace_back(m_db);
		m_db->move(155, 0);
	}
	void setModel(weak_ptr<HPModel::Node> nmodel) {
		auto modelCast = static_cast<HPReverbRingModel*>(nmodel.lock().get());
		m_wd->setModel(&modelCast->m_wd);
		m_clipDb->setModel(&modelCast->m_clipDb);
		m_fback->setModel(&modelCast->m_fback);
		m_factor->setModel(&modelCast->m_factor);
		m_passes->setModel(&modelCast->m_passes);
		m_db->setModel(&modelCast->m_db);
	}
private:
	Knob *m_wd;
	Knob *m_clipDb;
	Knob *m_fback;
	Knob *m_factor;
	LcdSpinBox *m_passes;
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
