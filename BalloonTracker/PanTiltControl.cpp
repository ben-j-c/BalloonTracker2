#include "PanTiltControl.h"

static SettingsWrapper sw;


void PTC::useSettings(SettingsWrapper& wrap) {
	sw = wrap;
}

void PTC::panCallback(int value, void*) {
	std::cout << "panCallback" << std::endl;
}

void PTC::tiltCallback(int value, void*) {
	std::cout << "tiltCallback" << std::endl;
}