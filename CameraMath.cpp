#include "CameraMath.h"
#include <opencv2/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif // !_USE_MATH_DEFINES

#include <math.h>


static double
	imHeight, //pixels
	imWidth, //pixels
	f, // mm
	sensorW, //mm
	sensorH, //mm
	balloonSpan, //mm
	sigma; //mm/px pixel size

static CameraMath::pos getCoordinates(double pxSize, double pxX, double pxY) {
	double z = CameraMath::calcDistance(pxSize);
	double x = sigma * pxX * z / f;
	double y = sigma * pxY * z / f;
	return { x,y,z };
}

void CameraMath::useSettings(SettingsWrapper & sw, int imageHeight, int imageWidth, double balloonCircumference) {
	imHeight = imageHeight;
	imWidth = imageWidth;
	f = sw.focal_length_min;
	sensorW = sw.sensor_width;
	sensorH = sw.sensor_height;
	balloonSpan = 10.0 * balloonCircumference * M_1_PI;
	sigma = sensorW / imWidth;
}

inline double CameraMath::calcDiameter(double area) {
	return sqrt(area * M_1_PI);
}

inline double CameraMath::calcDistance(double pxSize) {
	return balloonSpan * f / (pxSize * sigma); // mm
}

double CameraMath::calcRadialDistance(double pxSize, double pxX, double pxY) {
	auto r = getCoordinates(pxSize, pxX, pxY);
	return sqrt(r.z*r.z + r.x*r.x + r.y*r.y);
}

inline double CameraMath::calcPanRelative(double pxX) {
	return atan(sigma * pxX / f) * M_1_PI * 180.0;
}

inline double CameraMath::calcTiltRelative(double pxY) {
	return atan(sigma * pxY / f) * M_1_PI * 180.0;
}

CameraMath::pos CameraMath::calcRelativePos(double pan, double tilt, double radial) {
	pos returner;

	//Forward : +x
	//Left : +y
	//Up : +z
	returner.z = radial * sin(tilt * M_PI / 180.0);
	returner.x = radial * cos(tilt * M_PI / 180.0) * cos(pan * M_PI / 180.0);
	returner.y = radial * cos(tilt * M_PI / 180.0) * sin(pan * M_PI / 180.0);

	return returner;
}

CameraMath::pos CameraMath::calcRelativePos(double area, double pxX, double pxY, double pan, double tilt) {
	double pxSize = CameraMath::calcDiameter(area);
	pan += CameraMath::calcPanRelative(pxX);
	tilt += CameraMath::calcTiltRelative(pxY);
	double r = CameraMath::calcRadialDistance(pxSize, pxX, pxY);
	return CameraMath::calcRelativePos(pan, tilt, r);
}
