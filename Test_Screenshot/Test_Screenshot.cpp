#define _USE_MATH_DEFINES
#define _CRT_SECURE_NO_WARNINGS
#include "pch.h"

// Project
#include <FrameBuffer.h>
#include <SettingsWrapper.h>
#include <CameraMath.h>

// C/C++
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <math.h>
#include <chrono>
#include <ctime>

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
#include <opencv2/cudabgsegm.hpp>
#include <opencv2/cudacodec.hpp>
#include <opencv2/cudafeatures2d.hpp>
#include <opencv2/cudafilters.hpp>
#include <opencv2/cudaimgproc.hpp>
#include <opencv2/cudaobjdetect.hpp>
#include <opencv2/cudaoptflow.hpp>
#include <opencv2/cudastereo.hpp>
#include <opencv2/cudawarping.hpp>>

using namespace cv;
using namespace std;

// Global variables
FrameBuffer frameBuff(25);
SettingsWrapper sw("../settings.json");

bool setImage = false;
int screenShotIndex = 0;


char keyboard; //input from keyboard
void help() {
	cout
		<< "--------------------------------------------------------------------------" << endl
		<< "Used to take screenshots from the camera feed." << endl
		<< endl
		<< "Usage:" << endl
		<< "./Test_screenshot.exe" << endl
		<< "--------------------------------------------------------------------------" << endl
		<< endl;
}

void processFrames() {
	cuda::GpuMat gpuFrame, resized;
	Mat displayFrame;
	keyboard = 0;

	while (keyboard != 'q' && keyboard != 27) {
		Mat frame = frameBuff.getReadFrame();
		cuda::GpuMat gpuFrame(frame);
		frameBuff.readDone();

		cuda::resize(gpuFrame, resized,
			cv::Size(
				gpuFrame.cols*0.25,
				gpuFrame.rows*0.25));


		resized.download(displayFrame);
		imshow("Image", displayFrame);
		if (setImage) {
			auto date = chrono::system_clock::to_time_t(chrono::system_clock::now());
			string output = string("out") + std::ctime(&date) + ".png";
			imwrite(output, frame);
			imshow("Screenshot", displayFrame);
		}

		//get the input from the keyboard
		keyboard = (char)waitKey(1);
	}
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

void onClick(int ev, int x, int y, int flags, void*) {
	if (ev == cv::EVENT_LBUTTONDOWN) {
		setImage = true;
	}
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
	namedWindow("Image");
	namedWindow("Screenshot");
	cv::setMouseCallback("Image", onClick);

	//create Background Subtractor objects
	std::thread videoReadThread(processVideo, sw.camera);
	processFrames();
	videoReadThread.join();
	//destroy GUI windows
	destroyAllWindows();
	return EXIT_SUCCESS;
}
