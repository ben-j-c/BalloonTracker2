#include <SettingsWrapper.h>
#include <BalloonTrackerGUI2.1.h>
#include <thread>
#include <future>
#include <vector>
#include <iostream>

int main() {
	bool stop = false;
	SettingsWrapper sw("../settings.json");
	std::future<int> result = std::async(GUI::StartGUI, std::ref(sw), &stop);

	for (uint64_t i = 0; i < 400 && !stop; i++) {
		double a = (90 * sin((i / 1) / 100.0*3.14159*2.0));
		double b = (90 * cos((i / 1) / 100.0*3.14159*2.0));
		double c = (i*i / 10000.0*(a / 180.0 + 0.5));
		GUI::DataPoint& val = GUI::addData();
		val.index = i;
		val.mPan = a; val.mTilt = b;
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}

	result.get();
}