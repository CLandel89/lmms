INCLUDE(BuildPlugin)

BUILD_PLUGIN(hyperpipe
	HyperPipe.cpp HyperPipe.h settings.cpp synth.cpp view.cpp waveshapes.cpp
	MOCFILES HyperPipe.h
	EMBEDDED_RESOURCES *.png)
