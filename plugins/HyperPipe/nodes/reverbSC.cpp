/*
	reverbSC.cpp - implementation of the "reverbSC" node type (basically a copy of lmms::ReverbSCEffect)

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

extern "C" {
    #include "reverbSC/base.h"
    #include "reverbSC/revsc.h"
    #include "reverbSC/dcblock.h"
}

namespace lmms::hyperpipe
{

const string REVERB_SC_NAME = "reverbSC";

inline unique_ptr<HPNode> instantiateReverbSC(HPModel* model, int model_i);

struct HPReverbSCModel : public HPModel::Node {
	HPReverbSCModel(Instrument* instrument) :
			Node(instrument),
			m_wd(0.5f, 0.0f, 1.0f, 0.01f, instrument, QString("reverbSC w/d")),
			m_dbIn(0.0f, -60.0f, 15.0f, 0.1f, instrument, QString("reverbSC dB in")),
			m_size(0.89f, 0.0f, 1.0f, 0.01f, instrument, QString("reverbSC size")),
			m_color(10'000.0f, 100.0f, 15'000.0f, 1.0f, instrument, QString("reverbSC color")),
			m_dbOut(0.0f, -60.0f, 15.0f, 0.1f, instrument, QString("reverbSC dB out")),
			m_autoColor(false, instrument, QString("reverbSC auto color")),
			m_autoCorr(-2.0f, -30.0f, 30.0f, 0.1f, instrument, QString("reverbSC auto color +db p. oct."))
	{}
	FloatModel m_wd;
	FloatModel m_dbIn;
	FloatModel m_size;
	FloatModel m_color;
	FloatModel m_dbOut;
	BoolModel m_autoColor;
	FloatModel m_autoCorr;
	unique_ptr<HPNode> instantiate(HPModel* model, int model_i) {
		return instantiateReverbSC(model, model_i);
	}
	string name() { return REVERB_SC_NAME; }
	void load(int model_i, const QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_wd.loadSettings(elem, is + "_wd");
		m_dbIn.loadSettings(elem, is + "_dbIn");
		m_size.loadSettings(elem, is + "_size");
		m_color.loadSettings(elem, is + "_color");
		m_dbOut.loadSettings(elem, is + "_dbOut");
		m_autoColor.loadSettings(elem, is + "_autoColor");
		m_autoCorr.loadSettings(elem, is + "_autoCorr");
	}
	void save(int model_i, QDomDocument& doc, QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_wd.saveSettings(doc, elem, is + "_wd");
		m_dbIn.saveSettings(doc, elem, is + "_dbIn");
		m_size.saveSettings(doc, elem, is + "_size");
		m_color.saveSettings(doc, elem, is + "_color");
		m_dbOut.saveSettings(doc, elem, is + "_dbOut");
		m_autoColor.saveSettings(doc, elem, is + "_autoColor");
		m_autoCorr.saveSettings(doc, elem, is + "_autoCorr");
	}
	bool usesPrev() { return true; }
};

class HPReverbSC : public HPNode
{
public:
	HPReverbSC(HPModel* model, int model_i, shared_ptr<HPReverbSCModel> nmodel) :
			m_nmodel(nmodel),
			m_prev(model->instantiatePrev(model_i))
	{
		sp_create(&m_sp);
		m_srate_tmp = Engine::audioEngine()->outputSampleRate();
		m_sp->sr = m_srate_tmp;
		sp_revsc_create(&m_revsc);
		sp_revsc_init(m_sp, m_revsc);
		sp_dcblock_create(&m_dcblk[0]);
		sp_dcblock_create(&m_dcblk[1]);
		sp_dcblock_init(m_sp, m_dcblk[0], 1);
		sp_dcblock_init(m_sp, m_dcblk[1], 1);
	}
	~HPReverbSC() {
		sp_revsc_destroy(&m_revsc);
		sp_dcblock_destroy(&m_dcblk[0]);
		sp_dcblock_destroy(&m_dcblk[1]);
		sp_destroy(&m_sp);
	}
private:
	float processFrame(Params p) {
		if (m_srate_tmp != p.srate) {
			changeSampleRate(p.srate);
		}
		float buf = m_prev != nullptr
			? m_prev->processFrame(p)
			: 0.0f;
		float s[2] = {buf, buf};
		float w = m_nmodel->m_wd.value();
		float d = 1 - w;
		SPFLOAT tmpL, tmpR;
		SPFLOAT dcblkL, dcblkR;
		SPFLOAT inGain = powf(10.0f, m_nmodel->m_dbIn.value() / 20);
		SPFLOAT outGain = powf(10.0f, m_nmodel->m_dbOut.value() / 20);
		s[0] *= inGain;
		s[1] *= inGain;
		m_revsc->feedback = m_nmodel->m_size.value();
		float color = m_nmodel->m_color.value();
		if (m_nmodel->m_autoColor.value()) {
			float octs = log2f(p.freq / color);
			float db = m_nmodel->m_autoCorr.value() * octs;
			outGain *= powf(10, db / 20);
			color = p.freq;
		}
		m_revsc->lpfreq = color;
		sp_revsc_compute(m_sp, m_revsc, &s[0], &s[1], &tmpL, &tmpR);
		sp_dcblock_compute(m_sp, m_dcblk[0], &tmpL, &dcblkL);
		sp_dcblock_compute(m_sp, m_dcblk[1], &tmpR, &dcblkR);
		return 0.5f * (
			d * buf + w * dcblkL * outGain +
			d * buf + w * dcblkR * outGain
		);
	}
	void changeSampleRate(float srate) {
		// Change sr variable in Soundpipe. does not need to be destroyed
		m_srate_tmp = srate;
		m_sp->sr = m_srate_tmp;
		sp_revsc_destroy(&m_revsc);
		sp_dcblock_destroy(&m_dcblk[0]);
		sp_dcblock_destroy(&m_dcblk[1]);
		sp_revsc_create(&m_revsc);
		sp_revsc_init(m_sp, m_revsc);
		sp_dcblock_create(&m_dcblk[0]);
		sp_dcblock_create(&m_dcblk[1]);
		sp_dcblock_init(m_sp, m_dcblk[0], 1);
		sp_dcblock_init(m_sp, m_dcblk[1], 1);
	}
	void resetState() override {
		HPNode::resetState();
		if (m_prev != nullptr) {
			m_prev->resetState();
		}
		// provoke state reset
		m_srate_tmp = -1;
	}
	shared_ptr<HPReverbSCModel> m_nmodel;
	unique_ptr<HPNode> m_prev = nullptr;
	float m_srate_tmp;
	// Soundpipe C structs like in lmms::ReverbSCEffect
	sp_data *m_sp;
	sp_revsc *m_revsc;
	sp_dcblock *m_dcblk[2];
};

inline unique_ptr<HPNode> instantiateReverbSC(HPModel* model, int model_i) {
	return make_unique<HPReverbSC>(
		model,
		model_i,
		static_pointer_cast<HPReverbSCModel>(model->m_nodes[model_i])
	);
}

class HPReverbSCView : public HPNodeView {
public:
	HPReverbSCView(HPView* view) :
			m_wd(new Knob(view, "reverbSC w/d")),
			m_dbIn(new Knob(view, "reverbSC dB in")),
			m_size(new Knob(view, "reverbSC size")),
			m_color(new Knob(view, "reverbSC color")),
			m_dbOut(new Knob(view, "reverbSC dB out")),
			m_autoColor(new LedCheckBox(view, "reverbSC auto color")),
			m_autoCorr(new Knob(view, "reverbSC auto color +db p. oct."))
	{
		m_widgets.emplace_back(m_wd);
		m_dbIn->move(30, 0);
		m_widgets.emplace_back(m_dbIn);
		m_size->move(60, 0);
		m_widgets.emplace_back(m_size);
		m_color->move(90, 0);
		m_widgets.emplace_back(m_color);
		m_dbOut->move(120, 0);
		m_widgets.emplace_back(m_dbOut);
		m_autoColor->move(0, 30);
		m_widgets.emplace_back(m_autoColor);
		m_autoCorr->move(20, 30);
		m_widgets.emplace_back(m_autoCorr);
	}
	void setModel(weak_ptr<HPModel::Node> nmodel) {
		auto modelCast = static_cast<HPReverbSCModel*>(nmodel.lock().get());
		m_wd->setModel(&modelCast->m_wd);
		m_dbIn->setModel(&modelCast->m_dbIn);
		m_size->setModel(&modelCast->m_size);
		m_color->setModel(&modelCast->m_color);
		m_dbOut->setModel(&modelCast->m_dbOut);
		m_autoColor->setModel(&modelCast->m_autoColor);
		m_autoCorr->setModel(&modelCast->m_autoCorr);
	}
private:
	Knob *m_wd;
	Knob *m_dbIn;
	Knob *m_size;
	Knob *m_color;
	Knob *m_dbOut;
	LedCheckBox *m_autoColor;
	Knob *m_autoCorr;
};

using Definition = HPDefinition<HPReverbSCModel>;

template<> Definition::HPDefinition(HPInstrument* instrument) :
		HPDefinitionBase(instrument)
{
		m_forbidsArguments = true;
}

template<> Definition::~HPDefinition() = default;

template<> string Definition::name() { return REVERB_SC_NAME; }

template<> unique_ptr<HPReverbSCModel> Definition::newNodeImpl() {
	return make_unique<HPReverbSCModel>(m_instrument);
}

template<> unique_ptr<HPNodeView> Definition::instantiateView(HPView* hpview) {
	return make_unique<HPReverbSCView>(hpview);
}

} // namespace lmms::hyperpipe
