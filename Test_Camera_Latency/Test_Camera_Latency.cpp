// Project
#include <FrameBuffer.h>
#include <SettingsWrapper.h>

// C/C++
#include <limits>
#include <vector>
#include <queue>
#include <string>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <mutex>
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

using namespace cv;
using namespace std;

// Global variables
FrameBuffer frameBuff(25);
SettingsWrapper sw("../settings.json");
static int frameCount = 0;

char keyboard; //input from keyboard
void help() {
	cout
		<< "--------------------------------------------------------------------------" << endl
		<< "This program tests camera latency" << endl
		<< endl
		<< "Usage:" << endl
		<< "./Test_FeedbackLoop.exe" << endl
		<< "--------------------------------------------------------------------------" << endl
		<< endl;
}

void delay(int ms) {
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

auto timeNow() {
	return chrono::system_clock::now();
}

void backspace(int num) {
	for (int i = 0; i < num; i++) {
		cout << "\b \b";
	}
}

void processFrames() {
	int16_t* whitePtr = (int16_t*) calloc(640 * 480, sizeof(int16_t));
	int16_t* blackPtr = (int16_t*) calloc(640 * 480, sizeof(int16_t));

	memset(whitePtr, 255, 640 * 480);
	memset(blackPtr, 0, 640 * 480);

	Mat white(480, 640, CV_8U, (void*) whitePtr);
	Mat black(480, 640, CV_8U, (void*) blackPtr);
	keyboard = 0;

	auto startTime = timeNow();
	int startFrame = frameCount;
	bool flippedBlack = false;
	bool flippedWhite = true;

	imshow("Test_Camera_Latency", white);
	while (keyboard != 'q' && keyboard != 27) {
		Mat frame = frameBuff.getReadFrame();
		Mat roi(frame, Range(230, 250), Range(310, 330));
		Scalar s = mean(roi);

		imshow("ROI", roi);
		frameBuff.readDone();
		if ((s[0] + s[1] + s[2]) / 3 > 128 && flippedWhite) {
			chrono::duration<double> latency = timeNow() - startTime;
			cout << latency.count() * 1000 << " " << frameCount - startFrame << endl;
			flippedBlack = true;
			flippedWhite = false;

			imshow("Test_Camera_Latency", black);
		}
		else if((s[0] + s[1] + s[2]) / 3 < 128 && flippedBlack) {
			startFrame = frameCount;
			startTime = timeNow();
			flippedBlack = false;
			flippedWhite = true;
			imshow("Test_Camera_Latency", white);
		}

		frameCount++;
		keyboard = (char)waitKey(1);
	}

	free(whitePtr);
	free(blackPtr);
}

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
	namedWindow("Test_Camera_Latency");
	namedWindow("ROI");

	//Create reading thread
	std::thread videoReadThread(processVideo, sw.camera);
	//Jump to processing function
	processFrames();

	//On exit wait for reading thread
	videoReadThread.join();

	//destroy GUI windows
	destroyAllWindows();
	return EXIT_SUCCESS;
}