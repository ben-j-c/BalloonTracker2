#include "PanTiltControl.h"
#include <SerialPort.hpp>
#include <thread>
#include <string>

#define BUFF_LEN 1024

static SettingsWrapper sw;

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
	char readBuffer[BUFF_LEN] = { '\0' };
	string comPort = "\\\\.\\COM";
	comPort += std::to_string(sw.com_port);
	ardy = new SerialPort(comPort.data(), sw.com_baud);
	ardy->writeSerialPort("b", 1);
	for (int i = 0; readBuffer[0] != 'a'; i++) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		ardy->readSerialPort(readBuffer, BUFF_LEN);
		if (i == 10) {
			std::cerr << "Did not recieve ack from arduino!" << std::endl;
		}
	}
	std::cout << "Connection established" << std::endl;
}

void PTC::panCallback(int value, void*) {
	std::cout << "panCallback" << std::endl;
}

void PTC::tiltCallback(int value, void*) {
	std::cout << "tiltCallback" << std::endl;
}