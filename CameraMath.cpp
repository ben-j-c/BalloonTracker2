#include "CameraMath.h"

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif // !_USE_MATH_DEFINES

#include <math.h>

static double
	imHeight, //pixels
	imWidth, //pixels
	f, //mm
	sensorW, //mm
	sensorH, //mm
	balloonSpan, //mm
	sigma, //mm/px pixel size
	cx, //principal point x
	cy;

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
	cx = imWidth/2;
	cy = imHeight/2;
	balloonSpan = 10.0 * balloonCircumference * M_1_PI;
	sigma = sensorW / imWidth;
}

inline double CameraMath::calcDiameter(double area) {
	return sqrt(area * M_1_PI)*2;
}

inline double CameraMath::calcDistance(double pxSize) {
	return balloonSpan * f / (pxSize * sigma); // mm
}

double CameraMath::calcRadialDistance(double pxSize, double pxX, double pxY) {
	auto r = getCoordinates(pxSize, pxX, pxY);
	return sqrt(r.z*r.z + r.x*r.x + r.y*r.y);
}

CameraMath::pos CameraMath::calcAbsPos(double pxX, double pxY, double area, double pan, double tilt) {
	pos ret = calcDirection(pxX, pxY, pan, tilt);
	double pxSize = calcDiameter(area);
	double r = calcRadialDistance(pxSize, pxX, pxY);
	ret.x *= r;
	ret.y *= r;
	ret.z *= r;
	return ret;
}

CameraMath::pos CameraMath::calcDirection(double pxX, double pxY, double pan, double tilt) {
	double fPx = f / sigma; // focal length in pixels

	//Multiplication of augmented pixel location with inverse intrinsic
	double crx = pxX / fPx - cx / fPx; //camera relative x
	double cry = pxY / fPx - cy / fPx;
	double crz = 1.0;

	double theta = pan * M_PI / 180.0;
	double phi = tilt * M_PI / 180.0;

	//Multiplication by inverse extrinsic matrix
	double x = crx*cos(theta) + cry*sin(phi)*sin(theta) + crz*cos(phi)*sin(theta);
	double y = cry*cos(phi) - crz*sin(phi);
	double z = -crx*sin(theta) + cry*sin(phi)*cos(theta) + crz*cos(phi)*cos(theta);

	double mag = sqrt(x*x + y*y + z*z);

	return { x/mag, -y/mag, -z/mag };
}

double CameraMath::calcPan(CameraMath::pos &loc) {
	return atan(loc.x / loc.z)*180.0*M_1_PI;
}

double CameraMath::calcTilt(CameraMath::pos &loc) {
	return atan(loc.y / sqrt(loc.x*loc.x + loc.z*loc.z))*180.0*M_1_PI;
}