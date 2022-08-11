INCLUDE(BuildPlugin)

BUILD_PLUGIN(HyperPipe
	instrument.cpp HyperPipe.h model.cpp synth.cpp view.cpp
	nodes/fm.cpp nodes/noise.cpp nodes/sine.cpp nodes/shapes.cpp
	MOCFILES HyperPipe.h
	EMBEDDED_RESOURCES *.png)
