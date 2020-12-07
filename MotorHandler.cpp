#include "MotorHandler.h"
#include <include/SerialPort.hpp>

#include <thread>
#include <string>
#include <chrono>
#define _USE_MATH_DEFINES
#include <math.h>

#define BUFF_LEN 1024

static auto timeNow() {
	return std::chrono::system_clock::now();
}

static void delay(int ms) {
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

template<typename T>
T clamp(T a, T b, T v) {
	if (v < a)
		return a;
	if (v > b)
		return b;
	return v;
}

double MotorHandler::getCurrentPanDegrees() const {
	return (pan - sw.motor_pan_forward) / sw.motor_pan_factor;;
}

double MotorHandler::getCurrentTiltDegrees() const {
	return (tilt - sw.motor_tilt_forward) / sw.motor_tilt_factor;
}

void MotorHandler::setNextPanDegrees(double p) {
	double microseconds = sw.motor_pan_forward + p * sw.motor_pan_factor;
	pan = clamp<int>(sw.motor_pan_min, sw.motor_pan_max, microseconds);
}

void  MotorHandler::setNextTiltDegrees(double t) {
	double microseconds = sw.motor_tilt_forward + t * sw.motor_tilt_factor;
	tilt = clamp<int>(sw.motor_tilt_min, sw.motor_tilt_max, microseconds);
}

void MotorHandler::addPanDegrees(double p) {
	setNextPanDegrees(getCurrentPanDegrees() + p);
}

void MotorHandler::addTiltDegrees(double t) {
	setNextPanDegrees(getCurrentTiltDegrees() + t);
}

bool MotorHandler::startup() {
	char readBuffer[BUFF_LEN] = { '\0' };
	string comPort = "\\\\.\\COM";
	comPort += std::to_string(sw.com_port);
	ardy = new SerialPort(comPort.data(), sw.com_baud);
	if (!ardy->isConnected())
		return false;
	ardy->writeSerialPort("b", 1);
	for (int i = 0; readBuffer[0] != 'a'; i++) {
		ardy->readSerialPort(readBuffer, BUFF_LEN);
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		if (readBuffer[0] != 'a') {
			if (i >= sw.com_timeout / 100) {
				std::cout << "WARNING: Did not recieve ack from arduino! Stopping subsystem." << std::endl;
				return false;
			}
			else if(sw.print_info)
				std::cout << "INFO: Attempting to read ack again" << std::endl;
		}
	}
	if (readBuffer[0] == 'a') {
		if(sw.print_info)
			std::cout << "INFO: Connection established" << std::endl;
	}
	else
		return false;
	moveHome();
	engaged = true;
	std::cout << "Initial position:" << pan << " " << tilt << std::endl;
	samplingThread = std::thread(&MotorHandler::updatePos, this);
	return true;
}

void MotorHandler::moveHome() {
	pan = (int)(sw.motor_pan_forward);
	tilt = (int)(sw.motor_tilt_forward);
	writePos((int)sw.motor_pan_forward, (int)sw.motor_tilt_forward);
}

void MotorHandler::disengage() {
	char readBuffer[BUFF_LEN] = { '\0' };
	ardy->writeSerialPort("d", 1);
	for (int i = 0; readBuffer[0] != 'c'; i++) {
		ardy->readSerialPort(readBuffer, BUFF_LEN);
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		if (readBuffer[0] != 'c') {
			if (i == 10)
				std::cerr << "Did not recieve disengage ack from arduino!" << std::endl;
			else
				std::cout << "Attempting to read ack again" << std::endl;
		}
	}
	engaged = false;
}

double MotorHandler::panRange() const {
	return (sw.motor_pan_max - sw.motor_pan_min)/sw.motor_pan_factor;
}

double MotorHandler::tiltRange() const {
	return (sw.motor_tilt_max - sw.motor_tilt_min) / sw.motor_tilt_factor;
}

double MotorHandler::panMin() const {
	return (sw.motor_pan_min - sw.motor_pan_forward) / sw.motor_pan_factor;
}

double MotorHandler::panMax() const {
	return (sw.motor_pan_max - sw.motor_pan_forward) / sw.motor_pan_factor;
}

double MotorHandler::tiltMin() const {
	return (sw.motor_tilt_min - sw.motor_tilt_forward) / sw.motor_tilt_factor;
}

double MotorHandler::tiltMax() const {
	return (sw.motor_tilt_max - sw.motor_tilt_forward) / sw.motor_pan_factor;
}

MotorHandler::~MotorHandler() {
	killSignal = false;
	if(samplingThread.joinable())
		samplingThread.join();
}

void MotorHandler::updatePosRepeat() {
	constexpr auto updatePeriod = std::chrono::milliseconds(20);
	auto tNow = timeNow();
	while (!killSignal) {
		auto tNext = tNow + updatePeriod;
		writePos(pan, tilt);
		curPan = pan;
		curTilt = tilt;
		std::this_thread::sleep_until(tNext);
		tNow = tNext;
	}

	moveHome();
	delay(100);
	disengage();
}

void MotorHandler::updatePos() {
	writePos(pan, tilt);
	curPan = pan;
	curTilt = tilt;
}

void MotorHandler::writePos(int pan, int tilt) {
	pan = (int)max(min(pan, sw.motor_pan_max), sw.motor_pan_min);
	tilt = (int)max(min(tilt, sw.motor_tilt_max), sw.motor_tilt_min);
	pan = (int)(pan - sw.motor_pan_min);
	tilt = (int)(tilt - sw.motor_tilt_min);

	uint8_t writeBuffer[5];
	writeBuffer[0] = 'p';
	writeBuffer[1] = pan & 0xFF;
	writeBuffer[2] = (pan >> 8) & 0xFF;
	writeBuffer[3] = tilt & 0xFF;
	writeBuffer[4] = (tilt >> 8) & 0xFF;
	ardy->writeSerialPort((char*)writeBuffer, 5);
}

void MotorHandler::writePosShifted(int pan, int tilt) {
	writePos((int)(pan + sw.motor_pan_min), (int)(tilt + sw.motor_tilt_min));
}

bool MotorHandler::addRotation(double pan, double tilt) {
	return false;
}
