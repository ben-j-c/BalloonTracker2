#include <iostream>
#include <thread>
#include <SerialPort.hpp>
#include "SettingsWrapper.h"
#include <string>
#define BUFF_LEN 1024

using std::string;


void backspace(int n) {
	for (int i = 0; i < n; i++)
		std::cout << "\b \b";
}

int main()
{
	char readBuffer[BUFF_LEN] = { '\0' };
	SettingsWrapper sw("../settings.json");
	string comPort = "\\\\.\\COM";
	comPort += std::to_string(sw.com_port);
	SerialPort ardy(comPort.data(), sw.com_baud);
	ardy.writeSerialPort("b", 1);
	for (int i = 0; readBuffer[0] != 'a'; i++) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		ardy.readSerialPort(readBuffer, BUFF_LEN);
		if (i == 10) {
			std::cerr << "Did not recieve ack from arduino!" << std::endl;
		}
	}
	std::cout << "Connection established" << std::endl << "Starting test in 1 second..." << std::endl;
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	std::cout << "Test started" << std::endl;
	int panPattern[] = { 1200, 1200, 1800, 1800 };
	int tiltPattern[] = { 1200, 1800, 1800, 1200 };
	for (int i = 0; i < 3600; i++) {
		int pan = panPattern[i%4];
		int tilt = tiltPattern[i % 4];
		uint8_t writeBuffer[5];
		writeBuffer[0] = 'p';
		writeBuffer[1] = pan & 0xFF;
		writeBuffer[2] = (pan >> 8) & 0xFF;
		writeBuffer[3] = tilt & 0xFF;
		writeBuffer[4] = (tilt >> 8) & 0xFF;
		ardy.writeSerialPort((char*)writeBuffer, 5);
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		backspace(100);
		std::cout << "Time (Seconds):" << i << "/3600";
	}
	std::cout << "Test complete" << std::endl;
}