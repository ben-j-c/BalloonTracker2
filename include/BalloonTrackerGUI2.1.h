#pragma once
#include "SettingsWrapper.h"
#include <vector>
#include <chrono>


namespace GUI {
	struct DataPoint {
		double mPan, mTilt;
		double time;
		uint64_t index;
	};



	__declspec(dllimport) bool bStartSystemRequest;
	__declspec(dllimport) bool bSystemRunning;
	__declspec(dllimport) bool bStopSystemRequest;

	__declspec(dllimport) bool bStartImageProcRequest;
	__declspec(dllimport) bool bImageProcRunning;
	__declspec(dllimport) bool bStopImageProcRequest;

	__declspec(dllimport) bool bStartMotorContRequest;
	__declspec(dllimport) bool bMotorContRunning;
	__declspec(dllimport) bool bStopMotorContRequest;

	__declspec(dllimport) double dCountDown;
	__declspec(dllimport) double dCountDownValue;

	__declspec(dllimport) std::chrono::time_point<std::chrono::system_clock> tMeasurementStart;

	__declspec(dllimport) int StartGUI(SettingsWrapper& sw, bool* stop);

	__declspec(dllimport) void addData(const DataPoint& newData);
	__declspec(dllimport) DataPoint& addData();
};