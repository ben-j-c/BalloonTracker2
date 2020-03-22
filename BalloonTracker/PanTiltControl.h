#include <SettingsWrapper.h>
#include <iostream>
#include <string>

#pragma once
namespace PTC {
	extern int pan;
	extern int tilt;
	void useSettings(SettingsWrapper& sw);
	void writePos(int pan, int tilt);
	void moveHome();
	void disengage();
	void shutdown();
	void panCallback(int value, void*);
	void tiltCallback(int value, void*);
	bool addRotation(double pan, double tilt);
	int panRange();
	int tiltRange();
}

