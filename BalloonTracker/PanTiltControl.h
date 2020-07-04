#include <SettingsWrapper.h>
#include <iostream>
#include <string>
#include <functional>

#pragma once
namespace PTC {
	extern int pan;
	extern int tilt;
	bool useSettings(SettingsWrapper& wrap, const std::function<void(void)>& onStart);
	void writePos(int pan, int tilt);
	void writePosShifted(int pan, int tilt);
	void moveHome();
	void disengage();
	void shutdown();
	void panCallback(int value, void*);
	void tiltCallback(int value, void*);
	bool addRotation(double pan, double tilt);
	double currentPan();
	double currentTilt();
	int panRange();
	int tiltRange();
}

