INCLUDE(BuildPlugin)

BUILD_PLUGIN(HyperPipe
	instrument.cpp HyperPipe.h model.cpp synth.cpp view.cpp
	nodes/am.cpp nodes/amp.cpp nodes/env.cpp nodes/filter.cpp nodes/lfo.cpp nodes/fm.cpp nodes/mix.cpp nodes/noise.cpp nodes/organify.cpp nodes/overdrive.cpp nodes/reverbSC.cpp nodes/sine.cpp nodes/shapes.cpp nodes/square.cpp nodes/tune.cpp
	nodes/reverbSC/base.c nodes/reverbSC/revsc.c nodes/reverbSC/base.h nodes/reverbSC/revsc.h nodes/reverbSC/dcblock.h nodes/reverbSC/dcblock.c
	MOCFILES HyperPipe.h
	EMBEDDED_RESOURCES *.png)
