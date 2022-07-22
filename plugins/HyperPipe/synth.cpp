/*
 * synth.cpp - implementation for note handles and of simple synth nodes
 *
 * HyperPipe - synth with arbitrary possibilities
 *
 * Copyright (c) 2022 Christian Landel
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#include "HyperPipe.h"

namespace lmms
{

HyperPipeNode::HyperPipeNode()
{
}

HyperPipeNode::~HyperPipeNode()
{
}

HyperPipeOsc::HyperPipeOsc() :
		HyperPipeNode()
{
}

HyperPipeOsc::~HyperPipeOsc()
{
}

float HyperPipeOsc::processFrame(float freq, float srate) {
	m_ph += freq / srate;
	m_ph = fraction(m_ph);
	return shape(m_ph);
}

HyperPipeSine::HyperPipeSine(shared_ptr<HyperPipeModel::Sine> model) :
		m_sawify(model->m_sawify)
{
}

HyperPipeSine::~HyperPipeSine()
{
}

float HyperPipeSine::shape(float ph) {
	float s = sinf(ph * F_2PI);
	float saw = ph; //simplified and reversed
	saw = 1.0f - m_sawify->value() * saw; //ready for multiplication with s
	return saw * s;
}

HyperPipeNoise::HyperPipeNoise(shared_ptr<HyperPipeModel::Noise> model, Instrument* instrument) :
		m_spike(model->m_spike),
		m_osc(make_shared<HyperPipeModel::Sine>(instrument))
{
	m_osc.m_sawify->setValue(1.0f);
}

HyperPipeNoise::~HyperPipeNoise()
{
}

float HyperPipeNoise::processFrame(float freq, float srate) {
	float osc = m_osc.processFrame(freq, srate);
	osc = (osc + 1.0f) / 2.0f; //0.0...1.0
	osc = powf(osc, m_spike->value());
	float r = 1.0f - fastRandf(2.0f);
	return osc * r;
}

HyperPipeSynth::HyperPipeSynth(HyperPipe* instrument, NotePlayHandle* nph, HyperPipeModel* model) :
		m_instrument(instrument),
		m_nph(nph),
		m_lastNode(model->m_nodes.back()->instantiate(model->m_nodes.back()))
{
}

HyperPipeSynth::~HyperPipeSynth()
{
}

array<float,2> HyperPipeSynth::processFrame(float freq, float srate) {
	float f = m_lastNode->processFrame(freq, srate);
	return {f, f};
}

} // namespace lmms
