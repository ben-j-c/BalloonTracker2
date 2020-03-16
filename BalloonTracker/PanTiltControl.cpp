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

static void writePos(int pan, int tilt) {
	uint8_t writeBuffer[5];
	writeBuffer[0] = 'p';
	writeBuffer[1] = pan & 0xFF;
	writeBuffer[2] = (pan >> 8) & 0xFF;
	writeBuffer[3] = tilt & 0xFF;
	writeBuffer[4] = (tilt >> 8) & 0xFF;
	ardy->writeSerialPort((char*)writeBuffer, 5);
	delay(50);
}

void PTC::useSettings(SettingsWrapper& wrap) {
	sw = wrap;
	PTC::pan = sw.motor_pan_forward - sw.motor_pan_min;
	PTC::tilt = sw.motor_tilt_forward - sw.motor_tilt_min;
	std::cout << "Initial position:" << PTC::pan << " " << PTC::tilt << std::endl;

	char readBuffer[BUFF_LEN] = { '\0' };
	string comPort = "\\\\.\\COM";
	comPort += std::to_string(sw.com_port);
	ardy = new SerialPort(comPort.data(), sw.com_baud);
	ardy->writeSerialPort("b", 1);
	for (int i = 0; readBuffer[0] != 'a'; i++) {
		ardy->readSerialPort(readBuffer, BUFF_LEN);
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		if (readBuffer[0] != 'a') {
			if (i == 10)
				std::cerr << "Did not recieve ack from arduino!" << std::endl;
			else
				std::cout << "Attempting to read ack again" << std::endl;
		}
	}
	if (readBuffer[0] == 'a')
		std::cout << "Connection established" << std::endl;
	else
		exit(-1);
	delay(1000);
	writePos(PTC::pan + sw.motor_pan_min, PTC::tilt + sw.motor_tilt_min);
	delay(1000);
}

void PTC::panCallback(int value, void*) {
	writePos(PTC::pan + sw.motor_pan_min, PTC::tilt + sw.motor_tilt_min);
}

void PTC::tiltCallback(int value, void*) {
	writePos(PTC::pan + sw.motor_pan_min, PTC::tilt + sw.motor_tilt_min);
}

int PTC::panRange() {
	return  sw.motor_pan_max - sw.motor_pan_min;
}

int PTC::tiltRange() {
	return  sw.motor_tilt_max - sw.motor_tilt_min;
}