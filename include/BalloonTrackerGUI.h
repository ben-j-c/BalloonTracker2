#pragma once
#include "SettingsWrapper.h"
#include <vector>



namespace GUI {
	struct DataPoint {
		double x, y, z;
		double mPan, mTilt, pPan, pTilt;
		uint64_t index;
	};



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

	__declspec(dllimport) int StartGUI(SettingsWrapper& sw, bool* stop);

	__declspec(dllimport) void addData(const DataPoint& newData);
	__declspec(dllimport) DataPoint& addData();
};