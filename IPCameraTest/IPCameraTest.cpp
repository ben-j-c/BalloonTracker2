// IPCameraTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "pch.h"
#include "FrameBuffer.h"
#include "VideoReader.h"
#include <SettingsWrapper.h>

#include <vector>
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


char keyboard = 0; //input from keyboard
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
	
	std::chrono::high_resolution_clock timer;
	using milisec = std::chrono::duration<float, std::milli>;
	while (keyboard != 'q' && keyboard != 27) {
		Mat frame = frameBuff.getReadFrame();
		auto start = timer.now();

		cuda::GpuMat gpuFrame(frame);
		frameBuff.readDone();
		cuda::resize(gpuFrame, resized, cv::Size(gpuFrame.cols/4, gpuFrame.rows/4));
		resized.download(displayFrame);
		imshow("img", displayFrame);

		keyboard = (char)waitKey(1);

		auto stop = timer.now();
		float dt = std::chrono::duration_cast<milisec>(stop - start).count();
		frameTimesProc[index_proc] = dt;
		index_proc = (index_proc + 1) % 10;
	}
}

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
void processVideo(char* videoFilename) {
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
		auto start = timer.now();
		
		int errorCode = 0;
		if ((errorCode = vid.readFrame(buff)) < 0) {
			vid = VideoReader(videoFilename);
			consumeBufferedFrames(vid, buff);
			continue;
		}
		
		cv::cvtColor(frame, frame, CV_BGR2RGB);
		frameBuff.insertFrame(frame.clone());

		auto stop = timer.now();
		float dt = std::chrono::duration_cast<milisec>(stop - start).count();
		frameTimesImageAcq[index_acq] = dt;
		index_acq = (index_acq + 1) % 10;
	}
	keyboard = 'q';
	cout << "INFO: processVideo exiting." << endl;
	return;
}

void consoleUpdate() {
	while (keyboard != 'q' && keyboard != 27) {
		Scalar imageAcqFPS = cv::mean(frameTimesImageAcq);
		Scalar imageProcFPS = cv::mean(frameTimesProc);

		//for(int i = 0;i < 4;i++)
		//	cout << "\33[2K" << "\033[A" << "\r";

		/*
		cout << "Acquisition FPS: " << 1000.0/imageAcqFPS[0] << "             " << endl;
		cout << "Processing FPS: " << 1000.0/imageProcFPS[0] << "             " << endl;
		cout << "Blob mean: " << blobMean << "             " << endl;
		cout << "Buffer depth: " << frameBuff.size() << "             " << endl;
		*/
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
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
	std::thread videoReadThread(processVideo, argv[1]);
	std::thread consoleUpdateThread(consoleUpdate);
	processFrames();
	videoReadThread.join();
	consoleUpdateThread.join();
	//destroy GUI windows
	destroyAllWindows();
	return EXIT_SUCCESS;
}