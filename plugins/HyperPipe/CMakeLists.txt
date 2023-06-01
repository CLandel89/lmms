INCLUDE(BuildPlugin)

BUILD_PLUGIN(HyperPipe
	instrument.cpp HyperPipe.h model.cpp synth.cpp view.cpp
	nodes/am.cpp
	nodes/amp.cpp
	nodes/crush.cpp
	nodes/env.cpp
	nodes/filter.cpp
	nodes/leveler.cpp
	nodes/lfo.cpp
	nodes/fm.cpp
	nodes/mix.cpp
	nodes/noise.cpp
	nodes/organify.cpp
	nodes/overdrive.cpp
	nodes/reverbSC.cpp
	nodes/reverbSC/base.c nodes/reverbSC/revsc.c nodes/reverbSC/base.h nodes/reverbSC/revsc.h nodes/reverbSC/dcblock.h nodes/reverbSC/dcblock.c
	nodes/sine.cpp
	nodes/shapes.cpp
	nodes/square.cpp
	nodes/transition.cpp
	nodes/tune.cpp
	MOCFILES HyperPipe.h
	EMBEDDED_RESOURCES *.png)
