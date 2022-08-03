/*
	shapes.cpp - implementation of the "shapes" synth node

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

#include "HyperPipe.h"

inline float saw2tri(float ph, float morph)
{
	// left and right edge of each segment
	float le, re;
	le = 0.0f;
	re = morph * 0.25f;
	if (ph < re) {
		//0.0...1.0
		return ph / re;
	}
	le = re;
	re = 1.0f - morph * 0.25f;
	if (ph < re) {
		//this is the main (saw) shape
		//1.0...-1.0
		return 1.0f - 2.0f * (ph - le) / (re - le);
	}
	le = re;
	re = 1.0f;
	//-1.0...0.0
	return -1.0f + (ph - le) / (re - le);
}

inline float sqr2saw(float ph, float morph)
{
	// left and right edge of each segment
	float le, re;
	le = 0.0f;
	re = 0.5f - morph * 0.5f;
	if (ph < re) {
		return 1.0f;
	}
	le = re;
	re = 0.5f + morph * 0.5f;
	if (ph < re) {
		return 1.0f - 2.0f * (ph - le) / (re - le);
	}
	//re = 1.0f;
	return -1.0f;
}

inline float tri2sqr(float ph, float morph)
{
	// left and right edge of each segment
	float le, re;
	le = 0.0;
	re = 0.25f - morph * 0.25f;
	if (ph < re) {
		return ph / re;
	}
	//le = re;
	re = 0.25f + morph * 0.25f;
	if (ph < re) {
		return 1.0f;
	}
	le = re;
	re = 0.75f - morph * 0.25f;
	if (ph < re) {
		return 1.0f - 2.0f * (ph - le) / (re - le);
	}
	le = re;
	re = 0.75f + morph * 0.25f;
	if (ph < re) {
		return -1.0f;
	}
	le = re;
	re = 1.0f;
	return -1.0f + (ph - le) / (re - le);
}

namespace lmms::hyperpipe
{

HPShapes::HPShapes(shared_ptr<HPModel::Shapes> model) {
	if (model != nullptr) {
		m_shape = model->m_shape;
		m_jitter = model->m_jitter;
	}
}

float HPShapes::shape(float ph)
{
	float shape = m_shape != nullptr ? m_shape->value() : m_shape_fb;
	float jitter = m_jitter != nullptr ? m_jitter->value() : m_jitter_fb;
	//continuously add to the jittering
	m_jitterState = (127.0f * m_jitterState + fastRandf(jitter)) / 128.0f;
	float finalShape = shape + m_jitterState - 0.5f * jitter;
	while (finalShape < 0.0f) { finalShape += 3.0f; }
	while (finalShape >= 3.0f) { finalShape -= 3.0f; }
	float morph = fraction(finalShape);
	morph = sstep(morph);
	// amp: the shapes with vertical edges sound too loud
	if (finalShape < 1.0f) {
		float amp = 0.4f + 0.6f * morph;
		return amp * saw2tri(ph, morph);
	}
	if (finalShape < 2.0f) {
		float amp = 1.0f - 0.7f * morph;
		return amp * tri2sqr(ph, morph);
	}
	float amp = 0.3f + 0.1f * morph;
	return amp * sqr2saw(ph, morph);
}

} // namespace lmms::hyperpipe
