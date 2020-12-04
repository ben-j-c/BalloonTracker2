#pragma once
#include <SettingsWrapper.h>
#include <functional>

class MotorHandler {
public:
	MotorHandler(SettingsWrapper& s) : sw(s) {
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
	bool startup();
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
	std::thread samplingThread;

	//microseconds of PWM
	int pan, curPan;
	//microseconds of PWM
	int tilt, curTilt;
	bool engaged = false;
	bool killSignal = false;
	class SerialPort* ardy;
	void updatePosRepeat();
	void updatePos();
	void writePos(int pan, int tilt);
	void writePosShifted(int pan, int tilt);
	bool addRotation(double pan, double tilt);
};

