#ifdef _DEBUG
#endif


// Project
#include <CircularBuffer.h>
#include <SettingsWrapper.h>
#include <PanTiltControl.h>
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

CircularBuffer<ImageRes, 25> frameBuff;
#ifdef _DEBUG
SettingsWrapper sw("../settings.json");
#else
SettingsWrapper sw("./settings.json");
#endif // _DEBUG

//Global vars and handlers
bool bStop;
ZoomHandler zm(sw.onvifEndpoint);
bool bSendCoordinates = false;
bool bCountDownStarted = false;
double dBearing = 0;
std::chrono::time_point<std::chrono::system_clock> timeStart;

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

auto timeNow() {
	return chrono::system_clock::now();
}

void controlLoop() {
	while (!bStop) {
		if (GUI::bStartImageProcRequest) {
			GUI::bStartImageProcRequest = false;
			if (!GUI::bImageProcRunning) {
				//Start image processing
				GUI::bImageProcRunning = true;
			}
		}
		else if (GUI::bStopImageProcRequest) {
			GUI::bStopImageProcRequest = false;
			if (GUI::bImageProcRunning) {
				//Stop image processing
				GUI::bImageProcRunning = false;
			}
		}

		if (GUI::bStartMotorContRequest) {
			GUI::bStartMotorContRequest = false;
			if (!GUI::bMotorContRunning) {
				//Start motor control
				GUI::bMotorContRunning = true;
			}
		}
		else if (GUI::bStopMotorContRequest) {
			GUI::bStopMotorContRequest = false;
			if (GUI::bMotorContRunning) {
				//Stop motor control
				GUI::bMotorContRunning = false;
			}
		}

		if (GUI::bImageProcRunning) {
			//Do image processing
		}
		else {
			//Consume unused frames
			while(frameBuff.size()) {
				frameBuff.readDone();
			}
		}

	}
}

/*
	Consumes frames that were buffered by FFMPEG. Done to make the stream current and hence minimize delay.
*/
void consumeBufferedFrames(VideoReader& vid, ImageRes& buff) {
	using milisec = std::chrono::duration<float, std::milli>;
	std::chrono::high_resolution_clock timer;
	auto start = timer.now();
	vid.readFrame(buff);
	auto stop = timer.now();
	auto dt = std::chrono::duration_cast<milisec>(stop - start);

	//We need to drop all the frames we wont use.
	//We know that the frames pile up before we are able to process them.
	cout << "INFO: processVideo consuming frames." << endl;
	while (dt.count() < 1000.0f / 30) {
		this_thread::sleep_for(milisec(10));
		auto start = timer.now();
		vid.readFrame(buff);
		auto stop = timer.now();
		dt = std::chrono::duration_cast<milisec>(stop - start);
	}
	cout << "INFO: processVideo consuming frames. DONE" << endl;
}

/*
	Continually read frames from the stream and place them in the queue.
*/
void processVideo(char* videoFilename) {
	cout << "INFO: processVideo starting." << endl;
	VideoReader vid(videoFilename);
	cout << "INFO: processVideo starting. DONE" << endl;
	consumeBufferedFrames(vid, frameBuff.peekInsert());

	while (keyboard != 'q' && keyboard != 27) {
		ImageRes& buff = frameBuff.peekInsert();
		int errorCode = 0;
		if ((errorCode = vid.readFrame(buff)) < 0) {
			vid = VideoReader(videoFilename);
			consumeBufferedFrames(vid, buff);
			continue;
		}
		frameBuff.insertDone();
	}
	keyboard = 'q';
	cout << "INFO: processVideo exiting." << endl;
	return;
}

void countDownHandler() {
	while (!bStop) {
		while (!bStop && !GUI::bStartSystemRequest) {
			if (!GUI::bImageProcRunning && !GUI::bMotorContRunning)
				GUI::bStopSystemRequest = false;
			this_thread::yield();
			delay(10);
		}
		if (bStop)
			return;


		timeStart = timeNow();
		bCountDownStarted = true;
		while (!GUI::bStopSystemRequest && !bStop) {
			auto dt = timeNow() - timeStart;
			if (dt > std::chrono::duration<double, std::milli>(GUI::dCountDown*1000.0)) {
				if (sw.debug)
					std::cout << "INFO: System started." << std::endl;
				break;
			}
			GUI::dCountDownValue = (double)dt.count() / 1E7;
		}
		GUI::bStartSystemRequest = false; //Request to start has been processed
		GUI::bSystemRunning = true;
		bCountDownStarted = false;

		while ( //Wait for systems to shutdown.
			!bStop && !GUI::bStopSystemRequest &&
			(GUI::bImageProcRunning
				|| GUI::bMotorContRunning)) {
			this_thread::yield();
			delay(10);
		}
		GUI::bSystemRunning = false;
		GUI::bStartSystemRequest = false; //Pressing start does nothing when running
		GUI::bStopSystemRequest = false; //Request to stop has been processed
	}
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