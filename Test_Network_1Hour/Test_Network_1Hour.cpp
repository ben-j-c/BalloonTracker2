#include <iostream>
#include <SettingsWrapper.h>
#include <Network.h>
#include <vector>
#include <random>
#include <thread>


void backspace(int n) {
	for (int i = 0; i < n; i++)
		std::cout << "\b \b";
}

int main() {
	SettingsWrapper sw("../settings.json");
	Network::useSettings(sw);
	Network::startServer();
	std::cout << "Server started, waiting for client" << std::endl;
	Network::acceptConnection();
	std::cout << "Client connected" << std::endl;

	std::vector<double> data(25 * 3 * 3); //25 FPS, 3 doubles per frame, 3 seconds
	for (int i = 0; i < data.size(); i++) {
		data[i] = i;
	}
	std::cout << "Sending 3 second frame pack.";

	for (int i = 0; i < 1200; i++) {
		backspace(1000);
		std::cout << "Sending 3 second frame pack. " << i << "/" << "1200";
		if (Network::sendData(data) == -1)
			return -1;
		std::this_thread::sleep_for(std::chrono::milliseconds(3000));
	}
	return 0;
}
