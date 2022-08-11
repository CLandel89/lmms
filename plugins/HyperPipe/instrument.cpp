/*
	instrument.cpp - C export; implementation of classes HPInstrument and HPDefinitionBase

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

namespace lmms::hyperpipe
{

extern "C" {
	Plugin::Descriptor PLUGIN_EXPORT HyperPipe_plugin_descriptor = {
		LMMS_STRINGIFY(PLUGIN_NAME),
		"HyperPipe",
		QT_TRANSLATE_NOOP("PluginBrowser", "synth with arbitrary possibilities"),
		"Christian Landel",
		0x0110,
		Plugin::Instrument,
		new PluginPixmapLoader("logo"),
		nullptr,
		nullptr,
	};
	PLUGIN_EXPORT Plugin* lmms_plugin_main(Model* model, void*) {
		return new HPInstrument(static_cast<InstrumentTrack*>(model));
	}
}

template <typename M>
inline void createDefinition(
		map<string, unique_ptr<HPDefinitionBase>>& definitions,
		HPInstrument* instrument)
{
	unique_ptr<HPDefinitionBase> definition = make_unique<HPDefinition<M>>(instrument);
	definitions[definition->name()] = move(definition);
}

map<string, unique_ptr<HPDefinitionBase>> createDefinitions(HPInstrument* instrument) {
	map<string, unique_ptr<HPDefinitionBase>> definitions;
	createDefinition<HPFmModel>(definitions, instrument);
	createDefinition<HPNoiseModel>(definitions, instrument);
	createDefinition<HPShapesModel>(definitions, instrument);
	createDefinition<HPSineModel>(definitions, instrument);
	return definitions;
}

HPInstrument::HPInstrument(InstrumentTrack* instrumentTrack) :
		Instrument(instrumentTrack, &HyperPipe_plugin_descriptor),
		m_definitions(createDefinitions(this)),
		m_model(this)
{}

QString HPInstrument::nodeName() const {
	return HyperPipe_plugin_descriptor.name;
}

void HPInstrument::playNote(NotePlayHandle* nph, sampleFrame* working_buffer)
{
	if (nph->totalFramesPlayed() == 0 || nph->m_pluginData == nullptr) {
		if (nph->m_pluginData != nullptr) {
			delete static_cast<HPSynth*>(nph->m_pluginData);
		}
		nph->m_pluginData = new HPSynth(this, nph, &m_model);
	}
	HPSynth *synth = static_cast<HPSynth*>(nph->m_pluginData);
	const fpp_t frames = nph->framesLeftForCurrentPeriod();
	const f_cnt_t offset = nph->noteOffset();
	const float freq = nph->frequency();
	const float srate = Engine::audioEngine()->processingSampleRate();
	for (int i = 0; i < frames; i++) {
		working_buffer[offset + i] = synth->processFrame(freq, srate);
	}
	applyFadeIn(working_buffer, nph);
	applyRelease(working_buffer, nph);
	instrumentTrack()->processAudioBuffer(working_buffer, frames + offset, nph);
}

void HPInstrument::deleteNotePluginData(NotePlayHandle* nph) {
	delete static_cast<HPSynth*>(nph->m_pluginData);
}

void HPInstrument::saveSettings(QDomDocument& doc, QDomElement& elem) {
	IntModel size(m_model.size(), 0, 9999);
	size.saveSettings(doc, elem, "size");
	for (int i = 0; i < m_model.size(); i++) {
		QString is = "n" + QString::number(i);
		auto& node = m_model.m_nodes[i];
		node->m_pipe->saveSettings(doc, elem, is + "_pipe");
		IntModel nameHash(hphash(node->name()), INT16_MIN, INT16_MAX);
		nameHash.saveSettings(doc, elem, is + "_type");
		auto& arguments = node->m_arguments;
		IntModel argumentsSize(arguments.size(), 0, 9999);
		argumentsSize.saveSettings(doc, elem, is + "_arguments_size");
		for (int ia = 0; ia < arguments.size(); ia++) {
			QString ias = QString::number(ia);
			arguments[ia]->saveSettings(doc, elem, is + "_argument_" + ias);
		}
		node->save(i, doc, elem);
	}
}

void HPInstrument::loadSettings(const QDomElement& elem) {
	m_model.m_nodes.clear();
	map<int16_t, string> hash2name;
	for (auto& definition : m_definitions) {
		string name = definition.first;
		hash2name[hphash(name)] = name;
	}
	IntModel size(1, 0, 9999);
	size.loadSettings(elem, QString("size"));
	for (int i = 0; i < size.value(); i++) {
		QString is = "n" + QString::number(i);
		IntModel nameHash(0, INT16_MIN, INT16_MAX);
		nameHash.loadSettings(elem, is + "_type");
		//map::contains is introduced in c++20, so ... https://stackoverflow.com/a/3136537
		if (hash2name.find(nameHash.value()) != hash2name.end()) {
			string name = hash2name[nameHash.value()];
			m_model.m_nodes.emplace_back(
				m_definitions[name]->newNode()
			);
			m_model.m_nodes.back()->load(i, elem);
		}
		else {
			// let's hope this never happens
			m_model.m_nodes.emplace_back(
				m_definitions[HPDefinitionBase::DEFAULT_TYPE]->newNode()
			);
		}
		auto& node = m_model.m_nodes.back();
		node->m_pipe->loadSettings(elem, is + "_pipe");
		IntModel argumentsSize(0, 0, 9999);
		argumentsSize.loadSettings(elem, is + "_arguments_size");
		auto& arguments = node->m_arguments;
		for (int ia = 0; ia < argumentsSize.value(); ia++) {
			QString ias = QString::number(ia);
			shared_ptr<IntModel> argument = HPModel::newArgument(this, ia);
			argument->loadSettings(elem, is + "_argument_" + ias);
			arguments.emplace_back(argument);
		}
	}
}

gui::PluginView* HPInstrument::instantiateView(QWidget* parent) {
	return new HPView(this, parent);
}

void HPInstrument::chNodeType(string nodeType, int model_i)
{
	if (nodeType == m_model.m_nodes[model_i]->name()) {
		return;
	}
	m_model.m_nodes[model_i] = m_definitions[nodeType]->newNode();
}

HPDefinitionBase::HPDefinitionBase(HPInstrument* instrument) :
	m_instrument(instrument)
{}

} // namespace lmms::hyperpipe
