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
			m_wd(make_shared<FloatModel>(0.5f, 0.0f, 1.0f, 0.01f, instrument, QString("reverbSC w/d"))),
			m_dbIn(make_shared<FloatModel>(0.0f, -60.0f, 15.0f, 0.1f, instrument, QString("reverbSC dB in"))),
			m_size(make_shared<FloatModel>(0.89f, 0.0f, 1.0f, 0.01f, instrument, QString("reverbSC size"))),
			m_color(make_shared<FloatModel>(10'000.0f, 100.0f, 15'000.0f, 1.0f, instrument, QString("reverbSC color"))),
			m_dbOut(make_shared<FloatModel>(0.0f, -60.0f, 15.0f, 0.1f, instrument, QString("reverbSC dB out")))
	{}
	shared_ptr<FloatModel> m_wd;
	shared_ptr<FloatModel> m_dbIn;
	shared_ptr<FloatModel> m_size;
	shared_ptr<FloatModel> m_color;
	shared_ptr<FloatModel> m_dbOut;
	unique_ptr<HPNode> instantiate(HPModel* model, int model_i) {
		return instantiateReverbSC(model, model_i);
	}
	string name() { return REVERB_SC_NAME; }
	void load(int model_i, const QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_wd->loadSettings(elem, is + "_wd");
		m_dbIn->loadSettings(elem, is + "_dbIn");
		m_size->loadSettings(elem, is + "_size");
		m_color->loadSettings(elem, is + "_color");
		m_dbOut->loadSettings(elem, is + "_dbOut");
	}
	void save(int model_i, QDomDocument& doc, QDomElement& elem) {
		QString is = "n" + QString::number(model_i);
		m_wd->saveSettings(doc, elem, is + "_wd");
		m_dbIn->saveSettings(doc, elem, is + "_dbIn");
		m_size->saveSettings(doc, elem, is + "_size");
		m_color->saveSettings(doc, elem, is + "_color");
		m_dbOut->saveSettings(doc, elem, is + "_dbOut");
	}
};

class HPReverbSC : public HPNode
{
public:
	HPReverbSC(HPModel* model, int model_i, shared_ptr<HPReverbSCModel> nmodel) :
			m_wd(nmodel->m_wd),
			m_dbIn(nmodel->m_dbIn),
			m_size(nmodel->m_size),
			m_color(nmodel->m_color),
			m_dbOut(nmodel->m_dbOut),
			m_prev(model->instantiatePrev(model_i))
	{
		sp_create(&m_sp);
		m_srate_tmp = Engine::audioEngine()->processingSampleRate();
		m_sp->sr = m_srate_tmp;
		sp_revsc_create(&m_revsc);
		sp_revsc_init(m_sp, m_revsc);
		sp_dcblock_create(&m_dcblk[0]);
		//sp_dcblock_create(&m_dcblk[1]);
		sp_dcblock_init(m_sp, m_dcblk[0], Engine::audioEngine()->currentQualitySettings().sampleRateMultiplier());
		//sp_dcblock_init(sp, dcblk[1], Engine::audioEngine()->currentQualitySettings().sampleRateMultiplier());
	}
	~HPReverbSC() {
		sp_revsc_destroy(&m_revsc);
		sp_dcblock_destroy(&m_dcblk[0]);
		//sp_dcblock_destroy(&m_dcblk[1]);
		sp_destroy(&m_sp);
	}
private:
	float processFrame(float freq, float srate) {
		if (m_srate_tmp != srate) {
			changeSampleRate();
		}
		float buf = m_prev != nullptr
			? m_prev->processFrame(freq, srate)
			: 0.0f;
		float s[2] = {buf, buf};
		const float d = 1 - m_wd->value(), w = m_wd->value();
		SPFLOAT tmpL, tmpR;
		SPFLOAT dcblkL; //, dcblkR;
		const SPFLOAT inGain = powf(10.0f, m_dbIn->value() / 20);
		const SPFLOAT outGain = powf(10.0f, m_dbOut->value() / 20);
		s[0] *= inGain;
		s[1] *= inGain;
		m_revsc->feedback = m_size->value();
		m_revsc->lpfreq = m_color->value();
		sp_revsc_compute(m_sp, m_revsc, &s[0], &s[1], &tmpL, &tmpR);
		sp_dcblock_compute(m_sp, m_dcblk[0], &tmpL, &dcblkL);
		//sp_dcblock_compute(m_sp, m_dcblk[1], &tmpR, &dcblkR);
		buf = d * buf + w * dcblkL * outGain;
		//buf[f][1] = d * buf[f][1] + w * dcblkR * outGain;
		return buf;
	}
	void changeSampleRate() {
		// Change sr variable in Soundpipe. does not need to be destroyed
		m_srate_tmp = Engine::audioEngine()->processingSampleRate();
		m_sp->sr = m_srate_tmp;
		sp_revsc_destroy(&m_revsc);
		sp_dcblock_destroy(&m_dcblk[0]);
		//sp_dcblock_destroy(&m_dcblk[1]);
		sp_revsc_create(&m_revsc);
		sp_revsc_init(m_sp, m_revsc);
		sp_dcblock_create(&m_dcblk[0]);
		//sp_dcblock_create(&m_dcblk[1]);
		sp_dcblock_init(m_sp, m_dcblk[0], Engine::audioEngine()->currentQualitySettings().sampleRateMultiplier());
		//sp_dcblock_init(m_sp, m_dcblk[1], Engine::audioEngine()->currentQualitySettings().sampleRateMultiplier());
	}
	shared_ptr<FloatModel> m_wd;
	shared_ptr<FloatModel> m_dbIn;
	shared_ptr<FloatModel> m_size;
	shared_ptr<FloatModel> m_color;
	shared_ptr<FloatModel> m_dbOut;
	unique_ptr<HPNode> m_prev = nullptr;
	float m_srate_tmp;
	// Soundpipe C structs like in lmms::ReverbSCEffect
	sp_data *m_sp;
	sp_revsc *m_revsc;
	sp_dcblock *m_dcblk[1]; //mono
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
			m_wd(view, "reverbSC w/d"),
			m_dbIn(view, "reverbSC dB in"),
			m_size(view, "reverbSC size"),
			m_color(view, "reverbSC color"),
			m_dbOut(view, "reverbSC dB out")
	{
		m_widgets.emplace_back(&m_wd);
		m_dbIn.move(30, 0);
		m_widgets.emplace_back(&m_dbIn);
		m_size.move(60, 0);
		m_widgets.emplace_back(&m_size);
		m_color.move(90, 0);
		m_widgets.emplace_back(&m_color);
		m_dbOut.move(120, 0);
		m_widgets.emplace_back(&m_dbOut);
	}
	void setModel(shared_ptr<HPModel::Node> model) {
		shared_ptr<HPReverbSCModel> modelCast = static_pointer_cast<HPReverbSCModel>(model);
		m_wd.setModel(modelCast->m_wd.get());
		m_dbIn.setModel(modelCast->m_dbIn.get());
		m_size.setModel(modelCast->m_size.get());
		m_color.setModel(modelCast->m_color.get());
		m_dbOut.setModel(modelCast->m_dbOut.get());
	}
private:
	Knob m_wd;
	Knob m_dbIn;
	Knob m_size;
	Knob m_color;
	Knob m_dbOut;
};

using Definition = HPDefinition<HPReverbSCModel>;

template<> Definition::HPDefinition(HPInstrument* instrument) :
		HPDefinitionBase(instrument)
{
		m_forbidsArguments = true;
}

template<> Definition::~HPDefinition() = default;

template<> string Definition::name() { return REVERB_SC_NAME; }

template<> shared_ptr<HPReverbSCModel> Definition::newNodeImpl() {
	return make_shared<HPReverbSCModel>(m_instrument);
}

template<> unique_ptr<HPNodeView> Definition::instantiateView(HPView* hpview) {
	return make_unique<HPReverbSCView>(hpview);
}

} // namespace lmms::hyperpipe
