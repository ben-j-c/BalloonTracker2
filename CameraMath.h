#pragma once

#include "SettingsWrapper.h"

namespace CameraMath {
	/* Initialize camera math calculations
	sw : loaded from the settigs file
	imageHeight : number of rows in the image
	image width : number of cols in the image
	balloonCircumference : real world circumference of the balloon in centimeters
	*/
	void useSettings(SettingsWrapper &sw, int imageHeight, int imageWidth, double balloonCircumference);

	/* Calculate the radius of the balloon in pixels
	area : visual area of the balloon in pixels	
	*/
	double calcDiameter(double area);

	/* Calculate the distance from the focal point in the direction of the normal of the sensing plane (millimeters)
	pxSize : the visual diameter of the balloon
	*/
	double calcDistance(double pxSize);

	/* Calculate the radial distance from the focal point (millimeters)
	pxSize : the visual diameter of the balloon
	pxX : the column of the mean pixel of the balloon
	pxY : the row of the mean pixel of the balloon
	*/
	double calcRadialDistance(double pxSize, double pxX, double pxY);

	/* Calculate the pan rotation of the balloon relative to the camera
	pxX : the column of the mean pixel of the balloon
	*/
	double calcPanRelative(double pxX);

	/* Calculate the tilt rotation of the balloon relative to the camera
	pxY : the row of the mean pixel of the balloon
	*/
	double calcTiltRelative(double pxY);
	struct pos { double x, y, z; };

	/* Convert from a spherical coordinate system to cartesian coordinate system
	pan : degrees
	tilt : degrees
	radial : millimeters
	*/
	pos calcRelativePos(double pan, double tilt, double radial);

	/* Calculate the spherical coordinates of the balloon given its size and position
	area : visual area of the balloon in pixels
	pxX : the column of the mean pixel of the balloon
	pxY : the row of the mean pixel of the balloon
	pan : pan in degrees
	tilt : tilt in degrees
	*/
	pos calcRelativePos(double area, double pxX, double pxY, double pan, double tilt);
}