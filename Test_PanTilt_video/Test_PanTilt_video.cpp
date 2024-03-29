// IPCameraTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

// Project
#include <FrameBuffer.h>
#include "SettingsWrapper.h"
#include "PanTiltControl.h"

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
#include <opencv2/cudawarping.hpp>>

using namespace cv;
using namespace std;

// Global variables
FrameBuffer frameBuff(25);
SettingsWrapper sw("../settings.json");

int SThresh = 82;
int RThresh = 106;
int GThresh = 75;


char keyboard; //input from keyboard
void help() {
	cout
		<< "--------------------------------------------------------------------------" << endl
		<< "This program is used to calibrate the colour thresholds" << endl
		<< endl
		<< "Usage:" << endl
		<< "./IPCameraTest.exe" << endl
		<< "--------------------------------------------------------------------------" << endl
		<< endl;
}

void rgChroma(cuda::GpuMat image, std::vector<cuda::GpuMat>& chroma) {
	cuda::GpuMat i16;
	std::vector<cuda::GpuMat> colors(3);
	std::vector<cuda::GpuMat> rgc(3);

	//Convert to 16 bit and sum values
	image.convertTo(i16, CV_16UC3);
	cuda::split(i16, colors);
	cuda::add(colors[0], colors[1], chroma[3]);
	cuda::add(chroma[3], colors[2], chroma[3]);


	//perform fixed point rgChromaticity
	image.convertTo(i16, CV_16UC3, 256);
	cuda::split(i16, colors);
	cuda::divide(colors[0], chroma[3], rgc[0]);
	cuda::divide(colors[1], chroma[3], rgc[1]);
	cuda::divide(colors[2], chroma[3], rgc[2]);

	//Only lower bits are needed because of fixed point division
	rgc[0].convertTo(chroma[2], CV_8U);
	rgc[1].convertTo(chroma[1], CV_8U);
	rgc[2].convertTo(chroma[0], CV_8U);

	chroma[3].clone().convertTo(chroma[3], CV_8U, 1.0 / 3.0);
}

void processFrames() {
	cuda::GpuMat gpuFrame, resized, blob;
	std::vector<cuda::GpuMat> chroma(4);
	Mat displayFrame;
	keyboard = 0;

	while (keyboard != 'q' && keyboard != 27) {
		Mat frame = frameBuff.getReadFrame();
		cuda::GpuMat gpuFrame(frame);
		frameBuff.readDone();

		cuda::resize(gpuFrame, resized, cv::Size(gpuFrame.cols / 4, gpuFrame.rows / 4));
		rgChroma(resized, chroma);
		cuda::threshold(chroma[0], chroma[0], RThresh, 255, THRESH_BINARY);
		cuda::threshold(chroma[1], chroma[1], GThresh, 255, THRESH_BINARY_INV);
		cuda::threshold(chroma[3], chroma[3], SThresh, 255, THRESH_BINARY);

		cuda::bitwise_and(chroma[3], chroma[0], blob);
		cuda::bitwise_and(blob, chroma[1], blob);


		std::vector<cv::Point> loc;
		blob.download(displayFrame);
		cv::findNonZero(displayFrame, loc);
		resized.download(displayFrame);
		if (loc.size() > 50) {
			cv::Scalar mean = cv::mean(loc);
			double radius = sqrt(loc.size() / M_PI);
			cv::drawMarker(displayFrame, cv::Point2i(mean[0]+radius, mean[1]), cv::Scalar(255, 0, 0, 0), 0, 10, 1);
			cv::drawMarker(displayFrame, cv::Point2i(mean[0]-radius, mean[1]), cv::Scalar(255, 0, 0, 0), 0, 10, 1);
			cv::drawMarker(displayFrame, cv::Point2i(mean[0], mean[1]+radius), cv::Scalar(255, 0, 0, 0), 0, 10, 1);
			cv::drawMarker(displayFrame, cv::Point2i(mean[0], mean[1]-radius), cv::Scalar(255, 0, 0, 0), 0, 10, 1);
			string s = std::to_string(mean[0]) + " " + std::to_string(mean[1]);
			cv::putText(displayFrame, s, Point(10, 10), cv::FONT_HERSHEY_PLAIN, 2, cv::Scalar(255, 0, 255, 0));
		}
		else {
			cv::putText(displayFrame, "No blob found", Point(10, 10), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 0, 0, 0));
		}

		imshow("Image", displayFrame);

		blob.download(displayFrame);
		imshow("blob", displayFrame);

		//get the input from the keyboard
		keyboard = (char)waitKey(1);
	}
}

Mutex grabbingThread;

void grabFrames(VideoCapture *capture) {
	while(keyboard != 'q' && keyboard != 27 && capture->isOpened()) {
		if (grabbingThread.trylock()) {
			capture->grab();
			grabbingThread.unlock();
			std::this_thread::sleep_for(std::chrono::microseconds(500));
		}
	}
}

void processVideo(string videoFileName) {
	//create the capture object
	VideoCapture capture(videoFileName.data());
	capture.set(CAP_PROP_BUFFERSIZE, 2);
	capture.grab();
	std::thread grabber(grabFrames, &capture);
	Mat frame;
	while (keyboard != 'q' && keyboard != 27) {
		if (!capture.isOpened()) {
			//error in opening the video input
			cerr << "Unable to open video file: " << videoFileName << endl;
			exit(-1);
		}
		grabbingThread.lock();
		if (!capture.retrieve(frame)) {
			capture.release();
			capture.open(videoFileName);
			grabbingThread.unlock();
			continue;
		}
		grabbingThread.unlock();
		frameBuff.insertFrame(frame.clone());
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	grabbingThread.lock();
	capture.release();
	grabber.join();
	grabbingThread.unlock();
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
	namedWindow("blob");
	createTrackbar("S Thresh", "blob", &SThresh, 255, nullptr);
	createTrackbar("R Thresh", "blob", &RThresh, 255, nullptr);
	createTrackbar("G Thresh", "blob", &GThresh, 255, nullptr);

	PTC::useSettings(sw, [] {; });
	createTrackbar("Pan", "Image", &PTC::pan, PTC::panRange(), PTC::panCallback);
	createTrackbar("Tilt", "Image", &PTC::tilt, PTC::tiltRange(), PTC::tiltCallback);
	//create Background Subtractor objects
	std::thread videoReadThread(processVideo, sw.camera);
	processFrames();
	videoReadThread.join();
	//destroy GUI windows
	destroyAllWindows();
	PTC::shutdown();
	return EXIT_SUCCESS;
}