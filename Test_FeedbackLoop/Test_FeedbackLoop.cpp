// IPCameraTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

// Project
#include <FrameBuffer.h>
#include <SettingsWrapper.h>
#include <PanTiltControl.h>
#include <CameraMath.h>
#include <rapidjson/document.h>
#include <VideoReader.h>

// C/C++
#include <vector>
#include <queue>
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

struct MotorPos {
	double pan, tilt;
};

// Global variables
FrameBuffer frameBuff(25);
SettingsWrapper sw("../settings.json");
queue<MotorPos> positionBuffer;
CameraMath::pos lastPos;
static int frameCount = 0;
double initialDelay = 5;
double bearing = 0;

char keyboard; //input from keyboard
void help() {
	cout
		<< "--------------------------------------------------------------------------" << endl
		<< "This program tests the feedback loop" << endl
		<< endl
		<< "Usage:" << endl
		<< "./Test_FeedbackLoop.exe" << endl
		<< "--------------------------------------------------------------------------" << endl
		<< endl;
}

void delay(int ms) {
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void backspace(int num) {
	for (int i = 0; i < num; i++) {
		cout << "\b \b";
	}
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

/* Given the balloon size and position calculate where the motors should be and
*/
MotorPos sendData(double pxX, double pxY, double area) {
	MotorPos mp = positionBuffer.front();
	//positionBuffer.pop();

	CameraMath::pos pos = CameraMath::calcAbsPos(pxX, pxY, area, mp.pan, mp.tilt);
	double pan = CameraMath::calcPan(pos);
	double tilt = CameraMath::calcTilt(pos);

	//positionBuffer.push({ pan, tilt });

	if (frameCount != 0) {
		CameraMath::pos dv;
		dv.x = pos.x - lastPos.x;
		dv.y = pos.y - lastPos.y;
		dv.z = pos.z - lastPos.z;
		double time = frameCount / 25.0;
		double altitude = pos.y;
		double direction = atan2(-dv.z, dv.x)*180.0*M_1_PI + bearing;
		double speed = sqrt(dv.x*dv.x + dv.y*dv.y + dv.z*dv.z);

		cout << pos.x << " " << pos.y << " " << pos.z << endl;
		//cout << time << " " << altitude << " " << direction << " " << speed << endl;
	}

	lastPos = pos;

	return { pan, tilt };
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

		cuda::resize(gpuFrame, resized,
			cv::Size(
				gpuFrame.cols*sw.image_resize_factor,
				gpuFrame.rows*sw.image_resize_factor));
		rgChroma(resized, chroma);
		cuda::threshold(chroma[0], chroma[0], sw.thresh_red, 255, THRESH_BINARY);
		cuda::threshold(chroma[1], chroma[1], sw.thresh_green, 255, THRESH_BINARY_INV);
		cuda::threshold(chroma[3], chroma[3], sw.thresh_s, 255, THRESH_BINARY);

		cuda::bitwise_and(chroma[3], chroma[0], blob);


		std::vector<cv::Point> loc;
		blob.download(displayFrame);
		cv::findNonZero(displayFrame, loc);
		resized.download(displayFrame);
		if (loc.size() > 50) {
			cv::Scalar mean = cv::mean(loc);
			double pxRadius = sqrt(loc.size() / M_PI);
			cv::drawMarker(displayFrame, cv::Point2i(mean[0] + pxRadius, mean[1]), cv::Scalar(255, 0, 0, 0), 0, 10, 1);
			cv::drawMarker(displayFrame, cv::Point2i(mean[0] - pxRadius, mean[1]), cv::Scalar(255, 0, 0, 0), 0, 10, 1);
			cv::drawMarker(displayFrame, cv::Point2i(mean[0], mean[1] + pxRadius), cv::Scalar(255, 0, 0, 0), 0, 10, 1);
			cv::drawMarker(displayFrame, cv::Point2i(mean[0], mean[1] - pxRadius), cv::Scalar(255, 0, 0, 0), 0, 10, 1);

			double pxX = mean[0] / sw.image_resize_factor;
			double pxY = mean[1] / sw.image_resize_factor;
			double area = loc.size() / (sw.image_resize_factor * sw.image_resize_factor);

			MotorPos mp = sendData(pxX, pxY, area);
			cout << mp.pan << " " << mp.tilt << endl;
		}
		else {
			cout << "No pixels found" << endl;
		}

		imshow("Image", displayFrame);

		blob.download(displayFrame);
		imshow("blob", displayFrame);


		frameCount++;
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

	delay(1000);

	for (int i = 0; i < sw.motor_buffer_depth; i++) {
		positionBuffer.push({ 0,0 });
	}

	CameraMath::useSettings(sw, 78.5);

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