INCLUDE(BuildPlugin)

BUILD_PLUGIN(HyperPipe
	instrument.cpp HyperPipe.h model.cpp synth.cpp view.cpp
	nodes/am.cpp nodes/amp.cpp nodes/env.cpp nodes/lfo.cpp nodes/fm.cpp nodes/mix.cpp nodes/noise.cpp nodes/organify.cpp nodes/overdrive.cpp nodes/sine.cpp nodes/shapes.cpp nodes/tune.cpp
	MOCFILES HyperPipe.h
	EMBEDDED_RESOURCES *.png)
