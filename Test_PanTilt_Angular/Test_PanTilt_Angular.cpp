// Project
#include <FrameBuffer.h>
#include <SettingsWrapper.h>
#include <PanTiltControl.h>
#include <CameraMath.h>
#include <VideoReader.h>

// C/C++
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>

// OpenCV
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include <opencv2/highgui.hpp>
#include <opencv2/video.hpp>
#include <opencv2/photo.hpp>
#include <opencv2/features2d.hpp>

#include <opencv2/core/core.hpp>
#include <opencv2/core/cuda.hpp>
#include <opencv2/core/cuda.inl.hpp>
#include <opencv2/cudaarithm.hpp>
#include <opencv2/cudacodec.hpp>
#include <opencv2/cudafeatures2d.hpp>
#include <opencv2/cudafilters.hpp>
#include <opencv2/cudaimgproc.hpp>
#include <opencv2/cudawarping.hpp>

#define USE_VIDEOREADER_BEN

using namespace cv;
using namespace std;

// Global variables
FrameBuffer frameBuff(25);
SettingsWrapper sw("../settings.json");

int pan = 90;
int tilt = 0;

char keyboard; //input from keyboard
void help() {
	cout
		<< "--------------------------------------------------------------------------" << endl
		<< "This program verifies rotational control." << endl
		<< endl
		<< "Usage:" << endl
		<< "./Angular.exe" << endl
		<< "--------------------------------------------------------------------------" << endl
		<< endl;
}

void processFrames() {
	keyboard = 0;

	while (keyboard != 'q' && keyboard != 27) {
		//get the input from the keyboard
		keyboard = (char)waitKey(1);
	}
}

#ifdef USE_VIDEOREADER_BEN

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
void processVideo(const string& videoFilename) {
	cout << "INFO: processVideo starting." << endl;
	std::chrono::high_resolution_clock timer;
	using milisec = std::chrono::duration<float, std::milli>;
	VideoReader vid(videoFilename);
	ImageRes buff = vid.readFrame();
	cout << "INFO: processVideo starting. DONE" << endl;
	consumeBufferedFrames(vid, buff);

	//We need a Mat to be mapped to the buffer we are using to read frames into.
	Mat frame(vid.getHeight(), vid.getWidth(), CV_8UC3, buff.get());
	while (keyboard != 'q' && keyboard != 27) {
		int errorCode = 0;
		if ((errorCode = vid.readFrame(buff)) < 0) {
			vid = VideoReader(videoFilename);
			consumeBufferedFrames(vid, buff);
			continue;
		}

		cv::cvtColor(frame, frame, CV_BGR2RGB);
		frameBuff.insertFrame(frame.clone());
	}
	keyboard = 'q';
	cout << "INFO: processVideo exiting." << endl;
	return;
}

#else

void processVideo(string videoFileName) {

	//create the capture object
	VideoCapture capture(videoFileName.data());
	capture.set(CAP_PROP_BUFFERSIZE, 2);
	Mat frame;
	while (keyboard != 'q' && keyboard != 27) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		if (!capture.isOpened()) {
			//error in opening the video input
			cerr << "Unable to open video file: " << videoFileName << endl;
			exit(-1);
		}

		if (!capture.read(frame)) {
			capture.release();
			capture.open(videoFileName);
			continue;
		}
		frameBuff.insertFrame(frame.clone());
	}
	capture.release();
}

#endif

void tiltCallback(int, void*) {
	PTC::addRotation((pan - 90) - PTC::currentPan(), tilt - PTC::currentTilt());
}

void panCallback(int, void*) {
	PTC::addRotation((pan - 90) - PTC::currentPan(), tilt - PTC::currentTilt());
}

int main(int argc, char* argv[]) {
	//print help information
	help();
	//check for the input parameter correctness
	if (argc > 1) {
		cerr << "Incorrect input list" << endl;
		cerr << "exiting..." << endl;
		return EXIT_FAILURE;
	}

	//create GUI windows
	namedWindow("Angular");
	resizeWindow("Angular", 700, 100);
	PTC::useSettings(sw, [] {; });
	cv::createTrackbar("Pan Angle", "Angular", &pan, 180, panCallback);
	cv::createTrackbar("Tilt Angle", "Angular", &tilt, 90, tiltCallback);

	processFrames();
	PTC::shutdown();
	//destroy GUI windows
	destroyAllWindows();
	return EXIT_SUCCESS;
}