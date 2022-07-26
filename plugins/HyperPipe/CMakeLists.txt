INCLUDE(BuildPlugin)

BUILD_PLUGIN(HyperPipe
	instrument.cpp HyperPipe.h model.cpp shapes.cpp synth.cpp view.cpp
	MOCFILES HyperPipe.h
	EMBEDDED_RESOURCES *.png)
