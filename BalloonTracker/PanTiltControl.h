#include <SettingsWrapper.h>
#include <iostream>
#include <string>

#pragma once
namespace PTC {
	extern int pan;
	extern int tilt;
	void useSettings(SettingsWrapper& sw);
	void panCallback(int value, void*);
	void tiltCallback(int value, void*);
	int panRange();
	int tiltRange();
}

