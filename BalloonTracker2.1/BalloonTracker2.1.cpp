#ifdef _DEBUG
#endif


// Project
#include "FrameBuffer.h"
#include "SettingsWrapper.h"
#include "PanTiltControl.h"
#include <CameraMath.h>
#include <BalloonTrackerGUI.h>
#include <rapidjson/document.h>
#include <VideoReader.h>
#include <ZoomHandler.h>

// C/C++
#include <vector>
#include <queue>
#include <string>
#include <iostream>
#include <sstream>
#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <mutex>
#include <future>

// OpenCV
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include <opencv2/highgui.hpp>
#include <opencv2/video.hpp>
#include <opencv2/features2d.hpp>

#include <opencv2/core/core.hpp>
#include <opencv2/core/cuda.hpp>
#include <opencv2/core/cuda.inl.hpp>
#include <opencv2/cudaarithm.hpp>
#include <opencv2/cudaimgproc.hpp>
#include <opencv2/cudawarping.hpp>

#define USE_VIDEOREADER_BEN

using namespace cv;
using namespace std;

FrameBuffer frameBuff(25);
#ifdef _DEBUG
SettingsWrapper sw("../settings.json");
#else
SettingsWrapper sw("./settings.json");
#endif // _DEBUG

//Global vars and handlers
bool bStop;
ZoomHandler zm(sw.onvifEndpoint);


char keyboard; //input from keyboard
void help() {
	cout
		<< "--------------------------------------------------------------------------" << endl
		<< "This is the whole tracking subsystem." << endl
		<< endl
		<< "Usage:" << endl
		<< "./BalloonTracker.exe" << endl
		<< "--------------------------------------------------------------------------" << endl
		<< endl;

	cout << "Compilation details:" << endl;
	cout << "When: " << __TIMESTAMP__ << endl;
	cout << "Compiler version: " << __cplusplus << endl;
	cout << "x64: " << _M_X64 << endl;
}

void delay(int ms) {
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

int main(int argc, char* argv[]) {
	//print help information
	if (sw.print_info)
		help();
	//check for the input parameter correctness
	if (argc > 1) {
		cerr << "Incorrect input list" << endl;
		cerr << "exiting..." << endl;
		return EXIT_FAILURE;
	}

	//create GUI windows
	std::future<int> result = std::async(GUI::StartGUI, std::ref(sw), &bStop);
	//Start CV and motor subsystem handlers

	//On exit wait for reading thread
	result.get();
	bStop = true;

	//destroy GUI windows
	destroyAllWindows();
	return EXIT_SUCCESS;
}