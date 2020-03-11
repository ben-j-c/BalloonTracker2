// Test_PanTilt.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <thread>
#include <SerialPort.hpp>
#include "SettingsWrapper.h"
#include <string>
#define BUFF_LEN 1024

using std::string;


SettingsWrapper sw("../settings.json");
SerialPort* ardy;

void delay(int ms) {
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));

}

int main()
{
	std::cout << "Test starting in 5 seconds." << std::endl;
	delay(5000);
	std::cout << "Successfully loaded settings." << std::endl;
	char readBuffer[BUFF_LEN] = { '\0' };
	string comPort = "\\\\.\\COM";
	comPort += std::to_string(sw.com_port);
	delay(250);
	std::cout << "Using COM" << sw.com_port << std::endl;
	delay(250);
	std::cout << "Connecting to arduino." << std::endl;
	ardy = new SerialPort(comPort.data(), sw.com_baud);
	std::cout << "Connection established." << std::endl;
	delay(250);
	std::cout << "Verifying connection with ack signal." << std::endl;
	ardy->writeSerialPort("b", 1);
	std::cout << "ack request sent." << std::endl;
	for (int i = 0; readBuffer[0] != 'a'; i++) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		ardy->readSerialPort(readBuffer, BUFF_LEN);
		if (i == 10) {
			std::cerr << "Did not recieve ack from arduino! Test failed." << std::endl;
			return -1;
		}
		else if(readBuffer[0] != 'a') {
			std::cout << "No ack after " << i + 1 << " attempts. Trying again." << std::endl;
		}
	}
	std::cout << "Connection verified" << std::endl;
	delay(250);
	std::cout << "Test successful" << std::endl;
	return 0;
}

