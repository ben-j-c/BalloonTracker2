// IPCameraTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

// Project
#include "FrameBuffer.h"
#include "SettingsWrapper.h"
#include "PanTiltControl.h"
#include <CameraMath.h>
#include <BalloonTrackerGUI.h>
#include <rapidjson/document.h>

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


using namespace cv;
using namespace std;

struct MotorPos {
	double pan, tilt;
};

// Global variables
bool bStop = false;
FrameBuffer frameBuff(25);
#ifdef _DEBUG
SettingsWrapper sw("../settings.json");
#else
SettingsWrapper sw("./settings.json");
#endif // _DEBUG
bool bSendCoordinates = false;
bool bCountDownDone = false;
double dBearing = 0;

namespace Motor {
	bool inMotion, newDesire;
	//A buffer of flags denoting the motors are in motion
	queue<bool> motionHistory;
	//A buffer of motor positions. Buffer depth is given by number of frames of camera delay
	queue<MotorPos> history;
	mutex mutex_setPos, mutex_desiredPos;
	MotorPos setPos, desiredPos;
}

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
	cout << "When: " << __DATE__ << "at" << __LINE__ << endl;
	cout << "Compiler version: " << __cplusplus << endl;
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

static bool last;
/* Given the balloon size and position calculate where the motors should be.
Stand in for when data should be sent to the GUI.

Returns true when time to shutdown.
*/
bool sendData(double pxX, double pxY, double area) {
	static int iFrameCount = 0;
	Motor::mutex_setPos.lock();
	Motor::history.push(Motor::setPos);
	Motor::motionHistory.push(Motor::inMotion);
	Motor::mutex_setPos.unlock();
	MotorPos mp = Motor::history.front();
	bool inMotion = Motor::motionHistory.front();
	Motor::history.pop();
	Motor::motionHistory.pop();

	CameraMath::pos pos = CameraMath::calcAbsPos(pxX, pxY, area, -mp.pan, mp.tilt);
	double pan = CameraMath::calcPan(pos);
	double tilt = CameraMath::calcTilt(pos);

	if (inMotion != last && (bool)sw.debug) {
		cout << "Balloon pos:" << pan << " " << tilt << endl;
		cout << "Motor pos  :" << mp.pan << " " << mp.tilt << endl;
	}


	GUI::DataPoint& p = GUI::addData();
	p.index = iFrameCount;
	p.mPan = mp.pan + GUI::fBearing;
	p.mTilt = mp.tilt;
	p.pPan = pan + GUI::fBearing;
	p.pTilt = tilt;
	p.x = pos.x;
	p.y = pos.y;
	p.z = pos.z;

	if (sw.print_coordinates) {
		printf("(x,y,z): %f %f %f\n", pos.x, pos.y, pos.z);
	}

	if (sw.print_rotation) {
		printf("(pan,tilt): %f %f\n", pan, tilt);
	}

	//If the balloon has moved outside a 5 degree box, move to compensate
	if ((abs(mp.pan - pan) > 5 || abs(mp.tilt - tilt) > 5) && !inMotion) {
		Motor::mutex_desiredPos.lock();
		Motor::desiredPos = { pan, tilt };
		Motor::newDesire = true;
		Motor::mutex_desiredPos.unlock();
	}
	last = inMotion;

	iFrameCount++;
	return true;
}

/* Every 200 ms update the position of the motors.
Sets the motor position to the disired position, waits 200 ms, and then updates the set position
*/
void updatePTC() {
	//Wait for timer
	while (bCountDownDone && !bStop && !GUI::bStopMotorContRequest) {
		this_thread::yield();
		delay(50);
	}

	MotorPos mp = Motor::setPos;
	while (keyboard != 'q' && keyboard != 27 && !bStop && !GUI::bStopMotorContRequest) {
		Motor::mutex_desiredPos.lock();
		mp = Motor::desiredPos;
		bool nd = Motor::newDesire;
		Motor::mutex_desiredPos.unlock();

		if (nd) {
			PTC::addRotation(-mp.pan - PTC::currentPan(),
				mp.tilt - PTC::currentTilt());
			Motor::inMotion = true;
			if (sw.debug)
				cout << "Moving to:" << mp.pan << " " << mp.tilt << endl;

		}
		delay(240);

		Motor::mutex_setPos.lock();
		Motor::mutex_desiredPos.lock();
		if (Motor::inMotion)
			Motor::newDesire = false;
		Motor::inMotion = false;
		Motor::setPos = { -PTC::currentPan(), PTC::currentTilt() };
		Motor::mutex_desiredPos.unlock();
		Motor::mutex_setPos.unlock();
	}
}

/* Continually grab frames from the queue, perform the background subtraction,
and update the desired motor positions.
*/
void processFrames() {
	cuda::GpuMat gpuFrame, resized, blob;
	std::vector<cuda::GpuMat> chroma(4);
	Mat displayFrame;
	keyboard = 0;

	auto startTime = timeNow();
	while (keyboard != 'q' && keyboard != 27 && !bStop && !GUI::bStopImageProcRequest) {
		Mat frame;
		/*It is possible for the frame buffer to be empty when the video
		read thread exits (due to not being able to open the stream)*/
		try { frame = frameBuff.getReadFrame(); }
		catch (std::runtime_error r) {
			if (frameBuff.killSignal)
				continue;
			else
				throw r;
		}
		cuda::GpuMat gpuFrame(frame);
		frameBuff.readDone();



		cuda::resize(gpuFrame, resized,
			cv::Size(
			(int)(gpuFrame.cols*sw.image_resize_factor),
				(int)(gpuFrame.rows*sw.image_resize_factor)),
			0.0, 0.0, INTER_NEAREST);
		rgChroma(resized, chroma); //{R, G, B, S}
		cuda::threshold(chroma[0], chroma[0], sw.thresh_red, 255, THRESH_BINARY);
		cuda::threshold(chroma[1], chroma[1], sw.thresh_green, 255, THRESH_BINARY_INV);
		cuda::threshold(chroma[3], chroma[3], sw.thresh_s, 255, THRESH_BINARY);

		cuda::bitwise_and(chroma[3], chroma[0], blob);
		cuda::bitwise_and(chroma[1], blob, blob);


		std::vector<cv::Point> loc;
		blob.download(displayFrame);
		cv::findNonZero(displayFrame, loc);
		resized.download(displayFrame);
		if (loc.size() > 50) {
			cv::Scalar mean = cv::mean(loc);
			double pxRadius = sqrt(loc.size() / M_PI);
			cv::drawMarker(displayFrame, cv::Point2i((int)(mean[0] + pxRadius), (int)mean[1]),
				cv::Scalar(255, 0, 0, 0), 0, 10, 1);
			cv::drawMarker(displayFrame, cv::Point2i((int)(mean[0] - pxRadius), (int)mean[1]),
				cv::Scalar(255, 0, 0, 0), 0, 10, 1);
			cv::drawMarker(displayFrame, cv::Point2i((int)mean[0], (int)(mean[1] + pxRadius)),
				cv::Scalar(255, 0, 0, 0), 0, 10, 1);
			cv::drawMarker(displayFrame, cv::Point2i((int)mean[0], (int)(mean[1] - pxRadius)),
				cv::Scalar(255, 0, 0, 0), 0, 10, 1);

			double pxX = mean[0] / sw.image_resize_factor;
			double pxY = mean[1] / sw.image_resize_factor;
			double area = loc.size() / (sw.image_resize_factor * sw.image_resize_factor);

			if (bCountDownDone) {
				sendData(pxX, pxY, area);
			}
			else {
				std::chrono::duration<double> deltaT = (timeNow() - startTime);
				cv::putText(displayFrame, to_string(GUI::dCountDown - deltaT.count()),
					Point(5, 50), cv::FONT_HERSHEY_PLAIN, 1.5, cv::Scalar(255, 0, 255, 0), 2);
			}
		}
		else {
			if (bCountDownDone) {
				cout << "No pixels found" << endl;
				keyboard = 'q';
				break;
			}
		}

		if (sw.show_frame_rgb)
			imshow("Image", displayFrame);

		blob.download(displayFrame);
		if (sw.show_frame_mask)
			imshow("blob", displayFrame);

		if (sw.show_frame_mask_r)
			imshow("Mask R", chroma[0]);
		if (sw.show_frame_mask_g)
			imshow("Mask G", chroma[1]);
		if (sw.show_frame_mask_s)
			imshow("Mask S", chroma[3]);

		//get the input from the keyboard
		keyboard = (char)waitKey(1);
	}
}

void processVideo(string videoFileName, std::function<void(void)> onStart) {
	//create the capture object
	VideoCapture capture(videoFileName);
	capture.set(CAP_PROP_BUFFERSIZE, 2);
	onStart();
	Mat frame;
	while (keyboard != 'q' && keyboard != 27 && !bStop && !GUI::bStopImageProcRequest) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		if (!capture.isOpened()) {
			//error in opening the video input
			cerr << "Unable to open video file: " << videoFileName << endl;
			frameBuff.kill();
			break;
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

void motorRunningHandler() {
	for (uint32_t i = 0; i < sw.motor_buffer_depth; i++) {
		Motor::history.push({ 0, 0 });
		Motor::motionHistory.push(false);
	}

	while (!bStop) {
		//Wait for start button.
		while (!GUI::bStartMotorContRequest && !bStop) {
			this_thread::yield();
			delay(50);
		}

		PTC::useSettings(sw, [] {
			GUI::bStartMotorContRequest = false; //Request to start has been processed
			GUI::bMotorContRunning = true;
			});
		updatePTC(); //Jump to processing
		PTC::shutdown();
		GUI::bMotorContRunning = false;
		GUI::bStartMotorContRequest = false; //Pressing start does nothing when running
		GUI::bStopMotorContRequest = false; //Request to stop has been processed
	}
}

void videoRunningHandler() {
	while (!bStop) {
		//Wait for start button.
		while (!GUI::bStartImageProcRequest && !bStop) {
			this_thread::yield();
			delay(50);
		}

		frameBuff = FrameBuffer(25);
		std::thread videoReadThread(processVideo, sw.camera, [] {
			GUI::bStartImageProcRequest = false; //System started
			GUI::bImageProcRunning = true;
			});
		processFrames();
		videoReadThread.join();
		GUI::bImageProcRunning = false;
		GUI::bStartImageProcRequest = false; //Pressing start does nothing when running
		GUI::bStopImageProcRequest = false; //Request to stop has been processed
	}
}

void countDownHandler() {
	while (!bStop) {
		while (!GUI::bStartSystemRequest
			&& (!GUI::bImageProcRunning || !GUI::bMotorContRunning)
			&& !bStop) {
			this_thread::yield();
			delay(50);
		}

		GUI::bStartSystemRequest = false; //Request to start has been processed

		auto timeStart = timeNow();
		while (!GUI::bStopSystemRequest && !bStop) {
			bCountDownDone = (timeNow() - timeStart) > std::chrono::duration<double, std::milli>(GUI::dCountDown*1000.0);
			GUI::dCountDownValue = (double)(timeNow() - timeStart).count() / 1E9;
		}
		bCountDownDone = true;
		GUI::bSystemRunning = true;
		while (!GUI::bStopSystemRequest && !bStop) {
			this_thread::yield();
			delay(50);
		}
		GUI::bSystemRunning = false;
		bCountDownDone = false;
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
	GUI::StartGUI(sw, &bStop);
	if (sw.show_frame_rgb)
		namedWindow("Image");
	if (sw.show_frame_mask)
		namedWindow("blob");

	//Start CV and motor subsystem handlers
	std::thread motorsUpdateThread(motorRunningHandler);
	std::thread videoUpdateThread(videoRunningHandler);
	std::thread countDownThread(countDownHandler);

	//On exit wait for reading thread
	videoUpdateThread.join();
	motorsUpdateThread.join();

	//destroy GUI windows
	destroyAllWindows();
	return EXIT_SUCCESS;
}