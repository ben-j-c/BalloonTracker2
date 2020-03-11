#include <SettingsWrapper.h>
#include <iostream>
#include <string>

#pragma once
namespace PTC {
	void useSettings(SettingsWrapper& sw);
	void panCallback(int value, void*) {
		std::cout << "panCallback" << std::endl;
	}

	void tiltCallback(int value, void*) {
		std::cout << "tiltCallback" << std::endl;
	}
}

