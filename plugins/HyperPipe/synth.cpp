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

namespace lmms::hyperpipe
{

HPNode::HPNode()
{
}

HPNode::~HPNode()
{
}

HPOsc::HPOsc() :
		HPNode()
{
}

HPOsc::~HPOsc()
{
}

float HPOsc::processFrame(float freq, float srate) {
	m_ph += freq / srate;
	m_ph = fraction(m_ph);
	return shape(m_ph);
}

HPSine::HPSine(shared_ptr<HPModel::Sine> model) {
	if (model != nullptr) {
		m_sawify = model->m_sawify;
	}
}

HPSine::~HPSine()
{
}

float HPSine::shape(float ph) {
	float sawify = m_sawify != nullptr ? m_sawify->value() : m_sawify_fb;
	float s = sinf(ph * F_2PI);
	float saw = ph; //simplified and reversed
	saw = 1.0f - sawify * saw; //ready for multiplication with s
	return saw * s;
}

HPNoise::HPNoise(shared_ptr<HPModel::Noise> model) :
		m_spike(model->m_spike),
		m_osc(nullptr)
{
	if (model != nullptr) {
		m_spike = model->m_spike;
	}
	m_osc.m_sawify_fb = 1.0f;
}

HPNoise::~HPNoise()
{
}

float HPNoise::processFrame(float freq, float srate) {
	float spike = m_spike != nullptr ? m_spike->value() : m_spike_fb;
	float osc = m_osc.processFrame(freq, srate);
	osc = (osc + 1.0f) / 2.0f; //0.0...1.0
	osc = powf(osc, spike);
	float r = 1.0f - fastRandf(2.0f);
	return osc * r;
}

HPSynth::HPSynth(HyperPipe* instrument, NotePlayHandle* nph, HPModel* model) :
		m_instrument(instrument),
		m_nph(nph),
		m_lastNode(model->m_nodes.back()->instantiate(model->m_nodes.back()))
{
}

HPSynth::~HPSynth()
{
}

array<float,2> HPSynth::processFrame(float freq, float srate) {
	float f = m_lastNode->processFrame(freq, srate);
	return {f, f};
}

} // namespace lmms::hyperpipe
