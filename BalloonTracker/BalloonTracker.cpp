// IPCameraTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

// Project
#include "FrameBuffer.h"
#include "SettingsWrapper.h"
#include "PanTiltControl.h"
#include <CameraMath.h>
#include <Network.h>
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

//Avoid typos
#define CIRC "circumference"
#define DELAY "delay"
#define BEARING "bearing"
#define READY "ready"
#define MISSING "missing members"
#define MODE_SELECT "absCoord"

using namespace cv;
using namespace std;

struct MotorPos {
	double pan, tilt;
};

// Global variables
FrameBuffer frameBuff(25);
SettingsWrapper sw("../settings.json");
int frameCount = 0;
bool sendCoordinates = false;
chrono::seconds initialDelay;
double bearing = 0;

CameraMath::pos lastPos;
vector<double> outboundData;

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

	if (inMotion != last && (bool) sw.debug) {
		cout << "Balloon pos:" << pan << " " << tilt << endl;
		cout << "Motor pos  :" << mp.pan << " " << mp.tilt << endl;
	}

	if (frameCount != 0) {
		//compute desired params
		CameraMath::pos dv;
		dv.x = pos.x - lastPos.x;
		dv.y = pos.y - lastPos.y;
		dv.z = pos.z - lastPos.z;
		double time = frameCount / 25.0;
		double altitude = pos.y;
		double direction = atan2(-dv.z, dv.x)*180.0*M_1_PI + bearing;
		double speed = sqrt(dv.x*dv.x + dv.y*dv.y + dv.z*dv.z);

		//Every iteration store a column of data
		//Matlab uses column major for storing arrays
		if (sw.socket_enable) {
			if (sendCoordinates) {
				outboundData.push_back(time);
				outboundData.push_back(pos.x);
				outboundData.push_back(pos.y);
				outboundData.push_back(pos.z);
			}
			else {
				outboundData.push_back(time);
				outboundData.push_back(altitude);
				outboundData.push_back(direction);
				outboundData.push_back(speed);
			}

			//Send the number of columns followed by the columns
			if (frameCount % 50 == 0) {
				Network::sendData((int)(outboundData.size() / 4));
				Network::sendData(outboundData);
				outboundData.clear();
			}
		}
		else {
			if (sw.print_coordinates) {
				printf("(x,y,z): %f %f %f\n", pos.x, pos.y, pos.z);
			}

			if (sw.print_rotation) {
				printf("(pan,tilt): %f %f\n", pan, tilt);
			}
		}
	}
	lastPos = pos;

	//If the balloon has moved outside a 5 degree box, move to compensate
	if ((abs(mp.pan - pan) > 5 || abs(mp.tilt - tilt) > 5) && !inMotion) {
		Motor::mutex_desiredPos.lock();
		Motor::desiredPos = { pan, tilt };
		Motor::newDesire = true;
		Motor::mutex_desiredPos.unlock();
	}
	last = inMotion;

	if (Network::getBytesReady()) {
		return true;
	}
	return false;
}

/* Every 200 ms update the position of the motors.
Sets the motor position to the disired position, waits 200 ms, and then updates the set position
*/
void updatePTC() {
	auto startTime = timeNow();
	MotorPos mp = Motor::setPos;
	while ((timeNow() - startTime) < initialDelay) {
		this_thread::yield();
		delay(50);
	}
	while (keyboard != 'q' && keyboard != 27) {
		Motor::mutex_desiredPos.lock();
		mp = Motor::desiredPos;
		bool nd = Motor::newDesire;
		Motor::mutex_desiredPos.unlock();

		if (nd) {
			PTC::addRotation(-mp.pan - PTC::currentPan(),
				mp.tilt - PTC::currentTilt());
			Motor::inMotion = true;
			if(sw.debug)
				cout << "Moving to:" << mp.pan << " " << mp.tilt << endl;

		}
		delay(240);

		Motor::mutex_setPos.lock();
		Motor::mutex_desiredPos.lock();
		if (Motor::inMotion)
			Motor::newDesire = false;
		Motor::inMotion = false;
		Motor::setPos = {-PTC::currentPan(), PTC::currentTilt()};
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
	while (keyboard != 'q' && keyboard != 27) {
		Mat frame = frameBuff.getReadFrame();
		cuda::GpuMat gpuFrame(frame);
		frameBuff.readDone();

		cuda::resize(gpuFrame, resized,
			cv::Size(
				(int)(gpuFrame.cols*sw.image_resize_factor),
				(int)(gpuFrame.rows*sw.image_resize_factor)),
				0.0, 0.0, INTER_NEAREST);
		rgChroma(resized, chroma);
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
			cv::drawMarker(displayFrame, cv::Point2i((int) (mean[0] + pxRadius), (int) mean[1]),
				cv::Scalar(255, 0, 0, 0), 0, 10, 1);
			cv::drawMarker(displayFrame, cv::Point2i((int) (mean[0] - pxRadius), (int) mean[1]),
				cv::Scalar(255, 0, 0, 0), 0, 10, 1);
			cv::drawMarker(displayFrame, cv::Point2i((int) mean[0], (int) (mean[1] + pxRadius)),
				cv::Scalar(255, 0, 0, 0), 0, 10, 1);
			cv::drawMarker(displayFrame, cv::Point2i((int) mean[0], (int)(mean[1] - pxRadius)),
				cv::Scalar(255, 0, 0, 0), 0, 10, 1);

			double pxX = mean[0] / sw.image_resize_factor;
			double pxY = mean[1] / sw.image_resize_factor;
			double area = loc.size() / (sw.image_resize_factor * sw.image_resize_factor);

			bool stop = sendData(pxX, pxY, area);
			if (stop) {
				keyboard = 'q';
				break;
			}

			if ((timeNow() - startTime) >= initialDelay) {

			}
			else {
				std::chrono::duration<double> deltaT = (timeNow() - startTime);
				cv::putText(displayFrame, to_string(deltaT.count()),
					Point(5, 50), cv::FONT_HERSHEY_PLAIN, 1.5, cv::Scalar(255, 0, 255, 0), 2);
			}
		}
		else {
			if ((timeNow() - startTime) >= initialDelay) {
				cout << "No pixels found" << endl;
				keyboard = 'q';
				break;
			}
		}

		if(sw.show_frame_rgb)
			imshow("Image", displayFrame);

		blob.download(displayFrame);
		if(sw.show_frame_mask)
			imshow("blob", displayFrame);

		frameCount++;
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
			keyboard = 'q';
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

int main(int argc, char* argv[]) {
	//print help information
	if(sw.print_info)
		help();
	//check for the input parameter correctness
	if (argc > 1 ) {
		cerr << "Incorrect input list" << endl;
		cerr << "exiting..." << endl;
		return EXIT_FAILURE;
	}

	//create GUI windows
	if(sw.show_frame_rgb)
		namedWindow("Image");
	if(sw.show_frame_mask)
		namedWindow("blob");
	
	if (!sw.socket_enable) {
		cerr << "Error: Socket must have valid port. Exiting." << endl;
		exit(-1);
	}
	//Setup configuration
	Network::useSettings(sw);
	cout << "Starting server";
	cout << "on port " << sw.socket_port;
	cout << "..." << endl;
	Network::startServer();
	cout << "Server started, waiting for connection..." << endl;
	Network::acceptConnection();
	delay(250); //Wait so MATLAB can send input params
	std::vector<char> clientParams = Network::recvData();

	//Parse and validate the existance of params
	Document d;
	d.Parse(clientParams.data());
	if (!d.HasMember(CIRC) || 
		!d.HasMember(DELAY) || 
		!d.HasMember(BEARING) || 
		!d.HasMember(MODE_SELECT)) {
		Network::sendData(MISSING, strlen(MISSING));
		Network::shutdownServer();
		return EXIT_FAILURE;
	}
	else {
		Network::sendData(READY, strlen(READY));
	}

	if (sw.print_info) {
		cout << CIRC << " " << d[CIRC].GetDouble() << endl;
		cout << DELAY << " " << d[DELAY].GetDouble() << endl;
		cout << BEARING << " " << d[BEARING].GetDouble() << endl;
		cout << MODE_SELECT << " " << d[MODE_SELECT].GetBool() << endl;
	}

	initialDelay = chrono::seconds((int) d[DELAY].GetDouble());
	bearing = d[BEARING].GetDouble();
	CameraMath::useSettings(sw, sw.imH, sw.imW, d[CIRC].GetDouble());
	sendCoordinates = d[MODE_SELECT].GetBool();

	for (uint32_t i = 0; i < sw.motor_buffer_depth; i++) {
		Motor::history.push({ 0, 0 });
		Motor::motionHistory.push(false);
	}

	PTC::useSettings(sw);

	//Create reading thread
	std::thread videoReadThread(processVideo, sw.camera);
	std::thread motorsUpdateThread(updatePTC);
	//Jump to processing function
	processFrames();

	//On exit wait for reading thread
	videoReadThread.join();
	motorsUpdateThread.join();
	PTC::shutdown();

	//Send data done value
	Network::sendData(-1);
	delay(50);
	Network::shutdownServer();

	//destroy GUI windows
	destroyAllWindows();
	return EXIT_SUCCESS;
}