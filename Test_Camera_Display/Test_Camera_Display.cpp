// IPCameraTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "FrameBuffer.h"
#include <SettingsWrapper.h>

#include<vector>
//opencv
#include <opencv2/cvconfig.h>
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


char keyboard; //input from keyboard
void help() {
	cout
		<< "--------------------------------------------------------------------------" << endl
		<< "This program attempts to connect to the IP camera using OpenCV" << endl
		<< endl
		<< "Usage:" << endl
		<< "./Test_Camera_Display.exe" << endl
		<< "--------------------------------------------------------------------------" << endl
		<< endl;
}

void processFrames() {
	while (keyboard != 'q' && keyboard != 27) {
		Mat frame = frameBuff.getReadFrame();
		frameBuff.readDone();
		imshow("Frame", frame);

		//get the input from the keyboard
		keyboard = (char)waitKey(1);
	}
}

#if defined(_DEBUG)//!defined(HAVE_OPENCV_CUDACODEC)
void processVideo(std::string videoFileName) {
	//create the capture object
	VideoCapture capture;
	capture.set(cv::CAP_PROP_BUFFERSIZE, 0);
	capture.open(videoFileName, cv::CAP_FFMPEG);
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	Mat frame;
	std::chrono::high_resolution_clock timer;
	using milisec = std::chrono::duration<float, std::milli>;
	while (keyboard != 'q' && keyboard != 27) {
		auto start = timer.now();
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

		auto stop = timer.now();
		float dt = std::chrono::duration_cast<milisec>(stop - start).count();
		frameTimesImageAcq[index_acq] = dt;
		index_acq = (index_acq + 1) % 10;
	}
	capture.release();
}
#else

void processVideo(std::string videoFileName) {
	//create the capture object
	Ptr<cudacodec::VideoReader> d_reader = cv::cudacodec::createVideoReader(videoFileName);
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	cuda::GpuMat frame;
	std::chrono::high_resolution_clock timer;
	using milisec = std::chrono::duration<float, std::milli>;
	while (keyboard != 'q' && keyboard != 27) {
		if (!d_reader->nextFrame(frame)) {
			//error in opening the video input
			cerr << "Unable to open video file: " << videoFileName << endl;
			exit(-1);
		}
		Mat toQueue;
		frame.download(toQueue);
		frameBuff.insertFrame(toQueue);

		index_acq = (index_acq + 1) % 10;
	}

	d_reader.release();
}

#endif

int main(int argc, char* argv[]) {
	//print help information
	help();
	//check for the input parameter correctness
	if (argc != 1) {
		cerr << "Incorrect input list" << endl;
		cerr << "exiting..." << endl;
		return EXIT_FAILURE;
	}
	//create GUI windows
	namedWindow("Frame");

	std::thread videoReadThread(processVideo, sw.camera);
	processFrames();
	videoReadThread.join();
	//destroy GUI windows
	destroyAllWindows();
	return EXIT_SUCCESS;
}