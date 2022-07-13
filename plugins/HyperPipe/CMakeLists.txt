INCLUDE(BuildPlugin)

BUILD_PLUGIN(hyperpipe
	HyperPipe.cpp HyperPipe.h settings.cpp shapes.cpp synth.cpp view.cpp
	MOCFILES HyperPipe.h
	EMBEDDED_RESOURCES *.png)
