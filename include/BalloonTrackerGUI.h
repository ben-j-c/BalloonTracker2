#pragma once
#include "SettingsWrapper.h"
#include <vector>



namespace GUI {
	struct DataPoint {
		double x, y, z;
		double mPan, mTilt, pPan, pTilt;
		uint64_t index;
	};

	__declspec(dllimport) int StartGUI(SettingsWrapper& sw, bool* stop);
};

__declspec(dllimport) std::vector<GUI::DataPoint> data;

__declspec(dllimport) bool bStartSystemRequest;
__declspec(dllimport) bool bStopSystemRequest;
__declspec(dllimport) bool bStartImageProcRequest;
__declspec(dllimport) bool bStopImageProcRequest;
__declspec(dllimport) bool bStartMotorContRequest;
__declspec(dllimport) bool bStopMotorContRequest;
__declspec(dllimport) double dBalloonCirc;
__declspec(dllimport) double dCountDown;
__declspec(dllimport) double dCountDownValue;
__declspec(dllimport) float fBearing;

/*
extern __declspec(dllexport) std::vector<GUI::DataPoint> data;

extern __declspec(dllexport) bool bStartSystemRequest;
extern __declspec(dllexport) bool bStopSystemRequest;
extern __declspec(dllexport) bool bStartImageProcRequest;
extern __declspec(dllexport) bool bStopImageProcRequest;
extern __declspec(dllexport) bool bStartMotorContRequest;
extern __declspec(dllexport) bool bStopMotorContRequest;
extern __declspec(dllexport) double dBalloonCirc;
extern __declspec(dllexport) double dCountDown;
extern __declspec(dllexport) double dCountDownValue;
extern __declspec(dllexport) float fBearing;
*/