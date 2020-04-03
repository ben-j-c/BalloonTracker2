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
	Network::startServer();
	Network::acceptConnection();

	std::vector<double> data(25 * 3 * 3); //25 FPS, 3 doubles per frame, 3 seconds
	for (int i = 0; i < data.size(); i++) {
		data[i] = i;
	}
	for (int i = 0; i < 1200; i++) {
		std::cout << "Sending 3 second frame pack." << std::endl;
		Network::sendData(data);
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}
}
