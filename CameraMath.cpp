#include "CameraMath.h"
#include <opencv2/core.hpp>


static double imHeight, imWidth, f, sensorW, sensorH, balloonSpan;

void CameraMath::useSettings(SettingsWrapper & sw, int imageHeight, int imageWidth, double balloonCircumference) {
}

double CameraMath::calcRadius(double area) {
	return 0.0;
}

double CameraMath::calcDistance(double pxSize) {
	return 0.0;
}

double CameraMath::calcRadialDistance(double pxSize, double pxX, double pxY) {
	return 0.0;
}

double CameraMath::calcPanRelative(double pxX) {
	return 0.0;
}

double CameraMath::calcTiltRelative(double pxY) {
	return 0.0;
}

CameraMath::pos CameraMath::calcRelativePos(double pan, double tilt, double radial) {
	return pos();
}

CameraMath::pos CameraMath::calcRelativePos(double pxSize, double pxX, double pxY) {
	return pos();
}
