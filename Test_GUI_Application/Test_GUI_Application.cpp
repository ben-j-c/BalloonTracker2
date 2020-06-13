// Example interface to the GUI

#include <SettingsWrapper.h>
#include <BalloonTrackerGUI.h>
#include <thread>
#include <future>
#include <vector>


/*
__declspec(dllimport) std::vector<GUI::DataPoint> data;

__declspec(dllimport) bool bStartSystemRequest;
__declspec(dllimport) bool bStopSystemRequest;
__declspec(dllimport) bool bStartImageProcRequest;
__declspec(dllimport) bool bStopImageProcRequest;
__declspec(dllimport) bool bStartMotorContRequest;
__declspec(dllimport) bool bStopMotorContRequest;
__declspec(dllimport) double dBalloonCirc;
__declspec(dllimport) double dCountDown;
__declspec(dllimport) double dCountDownValue;
__declspec(dllimport) float fBearing;
*/

int main() {
	bool stop = false;
	SettingsWrapper sw("../settings.json");
	printf("%f\n", GUI::dBalloonCirc);
	std::future<int> result = std::async(GUI::StartGUI, std::ref(sw), &stop);

	for (uint64_t i = 0; i < 400; i++) {
		double a = (90 * sin((i / 1) / 100.0*3.14159*2.0));
		double b = (90 * cos((i / 1) / 100.0*3.14159*2.0));
		double c = (i*i / 10000.0*(a / 180.0 + 0.5));
		GUI::data.push_back(GUI::DataPoint{ 0.0, 1.0, 2.0,
			a, b, a*b / 90.0f, -a * b / 90.0f,
			(uint64_t)i });
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}

	result.get();
}
