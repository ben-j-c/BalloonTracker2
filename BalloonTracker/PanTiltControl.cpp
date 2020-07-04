#include "PanTiltControl.h"
#include "PanTiltControl.h"
#include <SerialPort.hpp>
#include <thread>
#include <string>

#define BUFF_LEN 1024

static SettingsWrapper sw;
int PTC::pan;
int PTC::tilt;

SerialPort* ardy;

using std::string;

static void backspace(int n) {
	for (int i = 0; i < n; i++)
		std::cout << "\b \b";
}

static void delay(int ms) {
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));

}

/* Write motor positions and perform clamping within min to max (motor_pan_min in settings.json).
*/
void PTC::writePos(int pan, int tilt) {
	pan = (int) max(min(pan, sw.motor_pan_max), sw.motor_pan_min);
	tilt = (int) max(min(tilt, sw.motor_tilt_max), sw.motor_tilt_min);
	PTC::pan = (int) (pan - sw.motor_pan_min);
	PTC::tilt = (int) (tilt - sw.motor_tilt_min);

	uint8_t writeBuffer[5];
	writeBuffer[0] = 'p';
	writeBuffer[1] = pan & 0xFF;
	writeBuffer[2] = (pan >> 8) & 0xFF;
	writeBuffer[3] = tilt & 0xFF;
	writeBuffer[4] = (tilt >> 8) & 0xFF;
	ardy->writeSerialPort((char*)writeBuffer, 5);
}

/* Write motor positions, but min value is 0 and max value is (max - min).
*/
void PTC::writePosShifted(int pan, int tilt) {
	PTC::writePos((int) (pan + sw.motor_pan_min),(int) (tilt + sw.motor_tilt_min));
}

/* Moves motors to motor_x_forward position (motor_pan_forward and motor_tilt_forward in settings.json).
*/
void PTC::moveHome() {
	PTC::writePos((int) sw.motor_pan_forward, (int) sw.motor_tilt_forward);
}

/* Disengage the motors. Checks to verify that the microcontroller received the signal.
*/
void PTC::disengage() {
	char readBuffer[BUFF_LEN] = { '\0' };
	ardy->writeSerialPort("d",1);
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
}

void PTC::shutdown() {
	PTC::moveHome();
	delay(500);
	PTC::disengage();
	delete ardy;
}

bool PTC::useSettings(SettingsWrapper& wrap, const std::function<void(void)>& onStart) {
	sw = wrap;
	PTC::pan = (int) (sw.motor_pan_forward - sw.motor_pan_min);
	PTC::tilt = (int) (sw.motor_tilt_forward - sw.motor_tilt_min);
	std::cout << "Initial position:" << PTC::pan << " " << PTC::tilt << std::endl;

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
			if (i == sw.com_timeout / 100) {
				std::cerr << "Did not recieve ack from arduino! Stopping subsystem." << std::endl;
				return false;
			}
			else
				std::cout << "Attempting to read ack again" << std::endl;
		}
	}
	if (readBuffer[0] == 'a')
		std::cout << "Connection established" << std::endl;
	else
		exit(-1);
	writePos((int)(PTC::pan + sw.motor_pan_min), (int)(PTC::tilt + sw.motor_tilt_min));
	onStart();
	delay(50);
	return true;
}

void PTC::panCallback(int value, void*) {
	writePosShifted(PTC::pan, PTC::tilt);
}

void PTC::tiltCallback(int value, void*) {
	writePosShifted(PTC::pan, PTC::tilt);
}

bool PTC::addRotation(double panDeg, double tiltDeg)
{
	bool returner = true;

	double deltaPan = panDeg * sw.motor_pan_factor;
	double deltaTilt = -tiltDeg * sw.motor_tilt_factor;

	int newPan = (int)(pan + deltaPan);
	int newTilt = (int)(tilt + deltaTilt);
	if (newPan + sw.motor_pan_min <= sw.motor_pan_max && newPan >= 0)
		pan = newPan;
	else
		returner = false;

	if (newTilt + sw.motor_tilt_min <= sw.motor_tilt_max && newTilt >= 0)
		tilt = newTilt;
	else
		returner = false;

	writePosShifted(pan, tilt);

	return returner;
}

double PTC::currentPan() {
	return (pan + sw.motor_pan_min - sw.motor_pan_forward) / sw.motor_pan_factor;
}

double PTC::currentTilt() {
	return -(tilt + sw.motor_tilt_min - sw.motor_tilt_forward) / sw.motor_tilt_factor;
}

int PTC::panRange() {
	return  (int)(sw.motor_pan_max - sw.motor_pan_min);
}

int PTC::tiltRange() {
	return  (int)(sw.motor_tilt_max - sw.motor_tilt_min);
}