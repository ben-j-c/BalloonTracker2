#pragma once
#include <SettingsWrapper.h>
#include <SerialPort.hpp>
#include <functional>

class MotorHandler {
public:
	MotorHandler(SettingsWrapper& s, bool* stop) : sw(s), killSignal(stop) {
		pan = sw.motor_tilt_forward;
		tilt = sw.motor_tilt_forward;
	}
	SettingsWrapper& sw;
	double getCurrentPanDegrees() const;
	double getCurrentTiltDegrees() const;
	void setNextPanDegrees(double p);
	void setNextTiltDegrees(double t);
	void addPanDegrees(double p);
	void addTiltDegrees(double t);
	void moveXYRelative(const std::pair<double, double>& dir);
	bool startup(const std::function<void(void)>& onStart);
	void moveHome();
	void disengage();
	double panRange() const;
	double tiltRange() const;
	double panMin() const;
	double panMax() const;
	double tiltMin() const;
	double tiltMax() const;

	~MotorHandler();
private:
	//microseconds of PWM
	int pan, curPan;
	//microseconds of PWM
	int tilt, curTilt;

	bool killSignal = false;
	SerialPort* ardy;
	void sampleThread();
	void writePos(int pan, int tilt);
	void writePosShifted(int pan, int tilt);
	bool addRotation(double pan, double tilt);
};

