#pragma once

#include "SettingsWrapper.h"

namespace CameraMath {
	void useSettings(SettingsWrapper &sw, int imageHeight, int imageWidth, double balloonCircumference);
	double calcRadius(double area);
	double calcDistance(double pxSize);
	double calcRadialDistance(double pxSize, double pxX, double pxY);
	double calcPanRelative(double pxX);
	double calcTiltRelative(double pxY);
	struct pos { double x, y, z; } calcRelativePos(double pan, double tilt, double radial);
}