#pragma once
#include "SettingsWrapper.h"
#include <vector>

namespace GUI {
	struct DataPoint{
		double x, y, z;
		double mPan, mTilt, pPan, pTilt;
		uint64_t index;
	};
	extern std::vector<DataPoint> data;

	extern bool bStartSystemRequest;
	extern bool bStopSystemRequest;
	extern bool bStartImageProcRequest;
	extern bool bStopImageProcRequest;
	extern bool bStartMotorContRequest;
	extern bool bStopMotorContRequest;
	extern double dBalloonCirc;
	extern double dCountDown;
	extern double dCountDownValue;
	extern float fBearing;

	int StartGUI(SettingsWrapper& sw);
}