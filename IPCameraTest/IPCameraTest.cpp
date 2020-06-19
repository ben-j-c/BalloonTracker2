// IPCameraTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "pch.h"
#include "FrameBuffer.h"
#include <SettingsWrapper.h>

#include<vector>
//opencv
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

//C
#include <stdio.h>
//C++
#include <iostream>
#include <sstream>
using namespace cv;
using namespace std;

// Global variables
FrameBuffer frameBuff(50);
int index_acq = 0;
int index_proc = 0;
vector<float> frameTimesImageAcq(10);
vector<float> frameTimesProc(10);
SettingsWrapper sw("./../settings.json");
Scalar blobMean;


char keyboard; //input from keyboard
void help() {
	cout
		<< "--------------------------------------------------------------------------" << endl
		<< "This program attempts to connect to the IP camera using OpenCV" << endl
		<< endl
		<< "Usage:" << endl
		<< "./IPCameraTest.exe <Location>" << endl
		<< "for example: ./IPCameraTest rtsp://192.168.2.168/ch1/main/av_stream" << endl
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

	chroma[3].clone().convertTo(chroma[3], CV_8U, 1.0/3.0);
}

void processFrames() {
	cuda::GpuMat gpuFrame, resized, blob;
	std::vector<cuda::GpuMat> chroma(4);
	Mat displayFrame;
	keyboard = 0;
	/*
	cv::SimpleBlobDetector::Params pr;
	pr.blobColor = 255;
	pr.filterByArea = true;
	pr.filterByCircularity = false;
	pr.filterByColor = false;
	pr.filterByConvexity = false;
	pr.
	Ptr<cv::SimpleBlobDetector> det = cv::SimpleBlobDetector::create();
	*/
	std::chrono::high_resolution_clock timer;
	using milisec = std::chrono::duration<float, std::milli>;
	while (keyboard != 'q' && keyboard != 27) {
		Mat frame = frameBuff.getReadFrame();
		auto start = timer.now();

		cuda::GpuMat gpuFrame(frame);
		frameBuff.readDone();

		cuda::resize(gpuFrame, resized, cv::Size(gpuFrame.cols/4, gpuFrame.rows/4));
		rgChroma(resized, chroma);
		cuda::threshold(chroma[0], chroma[0], sw.thresh_red, 255, THRESH_BINARY);
		cuda::threshold(chroma[3], chroma[3], sw.thresh_s, 255, THRESH_BINARY);

		cuda::bitwise_and(chroma[3], chroma[0], blob);
		

		std::vector<cv::Point> loc;
		blob.download(displayFrame);
		cv::findNonZero(displayFrame, loc);
		if(loc.size() > 0)
			blobMean = cv::mean(loc);

		auto stop = timer.now();
		float dt = std::chrono::duration_cast<milisec>(stop - start).count();
		frameTimesProc[index_proc] = dt;
		index_proc = (index_proc + 1) % 10;


		chroma[3].download(displayFrame);
		cv::drawMarker(displayFrame, Point2i(blobMean[0], blobMean[1]), Scalar(255, 0, 0, 0));
		imshow("S", displayFrame);

		chroma[0].download(displayFrame);
		imshow("R", displayFrame);

		chroma[1].download(displayFrame);
		imshow("G", displayFrame);

		chroma[2].download(displayFrame);
		imshow("B", displayFrame);

		blob.download(displayFrame);
		imshow("blob", displayFrame);

		//std::vector<cuda::GpuMat> splitImg{ chroma[2], chroma[1], chroma[0] };
		//cuda::merge(splitImg, gpuFrame);

		//get the input from the keyboard
		keyboard = (char)waitKey(1);
	}
}

void processVideo(char* videoFilename) {

	//create the capture object
	VideoCapture capture(videoFilename);
	Mat frame;
	std::chrono::high_resolution_clock timer;
	using milisec = std::chrono::duration<float, std::milli>;
	while (keyboard != 'q' && keyboard != 27) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		auto start = timer.now();
		if (!capture.isOpened()) {
			//error in opening the video input
			cerr << "Unable to open video file: " << videoFilename << endl;
			exit(-1);
		}

		if (!capture.read(frame)) {
			capture.release();
			capture.open(videoFilename);
			continue;
		}
		frameBuff.insertFrame(frame.clone());

		auto stop = timer.now();
		float dt = std::chrono::duration_cast<milisec>(stop - start).count();
		frameTimesImageAcq[index_acq] = dt;
		index_acq = (index_acq + 1) % 10;
	}
	capture.release();
}

void consoleUpdate() {
	while (keyboard != 'q' && keyboard != 27) {
		Scalar imageAcqFPS = cv::mean(frameTimesImageAcq);
		Scalar imageProcFPS = cv::mean(frameTimesProc);

		for(int i = 0;i < 3;i++)
			cout << "\33[2K" << "\033[A" << "\r";

		cout << "Acquisition FPS: " << 1000.0/imageAcqFPS[0] << "             " << endl;
		cout << "Processing FPS: " << 1000.0/imageProcFPS[0] << "             " << endl;
		cout << "Blob mean: " << blobMean << "             ";
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

int main(int argc, char* argv[]) {
	//print help information
	help();
	//check for the input parameter correctness
	if (argc != 2) {
		cerr << "Incorret input list" << endl;
		cerr << "exiting..." << endl;
		return EXIT_FAILURE;
	}
	//create GUI windows
	namedWindow("S");
	namedWindow("R");
	namedWindow("G");
	namedWindow("B");
	namedWindow("blob");
	//create Background Subtractor objects
	std::thread videoReadThread(processVideo, argv[1]);
	std::thread consoleUpdateThread(consoleUpdate);
	processFrames();
	videoReadThread.join();
	consoleUpdateThread.join();
	//destroy GUI windows
	destroyAllWindows();
	return EXIT_SUCCESS;
}