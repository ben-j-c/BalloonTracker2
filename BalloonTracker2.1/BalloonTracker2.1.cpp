#ifdef _DEBUG
#endif

// Project
#include <CircularBuffer.h>
#include <SettingsWrapper.h>
#include <PanTiltControl.h>
#include <CameraMath.h>
#include <BalloonTrackerGUI2.1.h>
#include <rapidjson/document.h>
#include <VideoReader.h>
#include <ZoomHandler.h>
#include <MotorHandler.h>

// C/C++
#include <vector>
#include <queue>
#include <string>
#include <iostream>
#include <sstream>
#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <future>

// OpenCV
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/video.hpp>
#include <opencv2/features2d.hpp>

#include <opencv2/core/core.hpp>
#include <opencv2/core/cuda.hpp>
#include <opencv2/core/cuda.inl.hpp>
#include <opencv2/cudaarithm.hpp>
#include <opencv2/cudaimgproc.hpp>
#include <opencv2/cudawarping.hpp>

#define USE_VIDEOREADER_BEN
#define PRINT_INFO(X) if(sw.print_info) cout << X << std::endl;
#define PRINT_WARN(X) if(sw.print_warnings) cout << X << std::endl;
#define PRINT_ERROR(X) if(sw.print_errors) cerr << X << std::endl;

using namespace cv;
using namespace std;

#ifdef _DEBUG
SettingsWrapper sw("../settings2.1.json");
#else
SettingsWrapper sw("./settings2.1.json");
#endif // _DEBUG

//Global vars and handlers
bool bStop = false;
CircularBuffer<ImageRes, 25> frameBuff;
ZoomHandler zoom(sw.onvifEndpoint);
MotorHandler motors(sw);
VideoReader vid;
std::thread vidThread;
std::thread vidStopThread;
std::thread motorsThread;
std::thread motorStopThread;
std::thread countdownStopThread;
std::thread countdownThread;
bool bSendCoordinates = false;
bool bCountDownStarted = false;
double dBearing = 0;
std::chrono::time_point<std::chrono::system_clock> timeStart;
std::future<int> guiResult;


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
	cout << "When: " << __TIMESTAMP__ << endl;
	cout << "Compiler version: " << __cplusplus << endl;
	cout << "x64: " << _M_X64 << endl;
}

void delay(int ms) {
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

auto timeNow() {
	return chrono::system_clock::now();
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

struct FindBalloonResult { double x, y; bool frameReady, noBalloon; };

struct FindBalloonResult findBalloon() {
	if (frameBuff.size() == 0)
		return { NAN, NAN, false, false };

	Mat frame = frameBuff.front();
	cuda::GpuMat gpuFrame(frame);
	frameBuff.pop_front();
	cuda::GpuMat resized, blob;
	std::vector<cuda::GpuMat> chroma(4);
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
	Mat temp;
	blob.download(temp);
	cv::findNonZero(temp, loc);

	Mat displayFrame;
	if (sw.show_frame_mask) {
		blob.download(displayFrame);
		imshow("blob", displayFrame);
	}
	if (sw.show_frame_mask_r) {
		chroma[0].download(displayFrame);
		imshow("Mask R", displayFrame);
	}
	if (sw.show_frame_mask_g) {
		chroma[1].download(displayFrame);
		imshow("Mask G", displayFrame);
	}
	if (sw.show_frame_mask_s) {
		chroma[3].download(displayFrame);
		imshow("Mask S", displayFrame);
	}

	if (loc.size() >= 50) {
		cv::Scalar mean = cv::mean(loc);
		double pxRadius = sqrt(loc.size() / M_PI);
		double pxX = mean[0] / sw.image_resize_factor;
		double pxY = mean[1] / sw.image_resize_factor;
		double area = loc.size() / (sw.image_resize_factor * sw.image_resize_factor);

		if (sw.show_frame_rgb) {
			resized.download(displayFrame);
			if (sw.show_frame_track) {
				cv::drawMarker(displayFrame, cv::Point2i((int)(mean[0] + pxRadius), (int)mean[1]),
					cv::Scalar(255, 0, 0, 0), 0, 10, 1);
				cv::drawMarker(displayFrame, cv::Point2i((int)(mean[0] - pxRadius), (int)mean[1]),
					cv::Scalar(255, 0, 0, 0), 0, 10, 1);
				cv::drawMarker(displayFrame, cv::Point2i((int)mean[0], (int)(mean[1] + pxRadius)),
					cv::Scalar(255, 0, 0, 0), 0, 10, 1);
				cv::drawMarker(displayFrame, cv::Point2i((int)mean[0], (int)(mean[1] - pxRadius)),
					cv::Scalar(255, 0, 0, 0), 0, 10, 1);
			}
			imshow("Image", displayFrame);
		}

		return { pxX, pxY, true, false };
	}
	else {
		if (sw.show_frame_rgb) {
			resized.download(displayFrame);
			imshow("Image", displayFrame);
		}
		
		return { NAN, NAN, true, true };
	}
}

void countdownHandler() {
	using namespace std::chrono;
	timeStart = timeNow();
	while (!GUI::bStopSystemRequest && !bStop) {
		auto dt = timeStart - timeNow();
		if (dt > std::chrono::duration<double, std::milli>(GUI::dCountDown*1000.0)) {
			PRINT_INFO("INFO: Count down ended.");
			break;
		}
		GUI::dCountDownValue = (double)dt.count() / 1E7;
		delay(1);
	}
}

/*
	Consumes frames that were buffered by FFMPEG. Done to make the stream current and hence minimize delay.
*/
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
		this_thread::sleep_for(milisec(10));
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
void processVideo(const string& videoFilename, const std::function<void()>& onStart) {
	PRINT_INFO("INFO: processVideo starting.");
	vid = VideoReader(videoFilename);
	PRINT_INFO("INFO: processVideo starting. DONE");
	consumeBufferedFrames(vid, frameBuff.back());
	onStart();
	while (!bStop && !GUI::bStopImageProcRequest) {
		if (frameBuff.size() == frameBuff.capacity() - 1) {
			if (sw.print_warnings)
				cout << "WARNING: Dropping frames due to full frame buffer!" << std::endl;
			static ImageRes frameDrop;
			vid.readFrame(frameDrop);
			delay(1);
			continue;
		}
		ImageRes& buff = frameBuff.back();
		int errorCode = 0;
		if ((errorCode = vid.readFrame(buff)) < 0) {
			if (sw.print_warnings)
				cout << "WARNING: Video stream dropped! Attempting re-connection." << std::endl;
			vid = VideoReader(videoFilename);
			consumeBufferedFrames(vid, buff);
			continue;
		}
		frameBuff.push_back();
	}
	keyboard = 'q';
	if (sw.print_info)
		cout << "INFO: processVideo exiting." << endl;
	return;
}

void stateChange() {
	static bool bMotorStarting = false, bImageProcStarting = false, bSystemStarting = false;
	static bool bMotorStopping = false, bImageProcStopping = false, bSystemStopping = false;
	/* breakdown of signals
		GUI::bStartImageProcRequest : GUI start image processing button pressed
		GUI::bStopImageProcRequest  : GUI stop image processing button pressed
		GUI::bImageProcRunning      : image processing running; set true on negative edge of bImageProcStarting; set false on negative edge of bImageProcStopping
		bImageProcStarting          : set true when bStartImageProcRequest positive edge detected and bImageProcRunning is false; set false after initialization
		bImageProcStopping          : bStopImageProcRequest positive edge detected and bImageProcRunning is true; set false after deconstruction

		GUI::bStartMotorContRequest : GUI start motor control button pressed
		GUI::bStopMotorContRequest  : GUI stop motor control button pressed
		GUI::bMotorContRunning      : motor control running; set true on negative edge of bMotorContStarting; set false on negative edge of bMotorContStopping
		bMotorContStarting          : set true when bStartMotorContRequest positive edge detected and bMotorContRunning is false; set false after initialization
		bMotorContStopping          : bMotorContRequest positive edge detected and bMotorContRunning is true; set false after deconstruction

		GUI::bStartSystemRequest    : GUI start system button pressed
		GUI::bStopSystemRequest     : GUI stop system button pressed
		GUI::bSystemRunning         : system is running; set true on negative edge on bSystemStarting
		bSystemStarting             : bMotorContStarting or bImageProcStarting or countdown not done
		bSystemStopping             : bMotorContStopping or bImageProcStopping or bStopSystemRequest
		*/
	if (GUI::bStartImageProcRequest) {
		if (!GUI::bImageProcRunning && !bImageProcStarting) {
			//Start image processing
			bImageProcStarting = true;
			PRINT_INFO("INFO: Image processing starting.");
			vidThread = std::thread(processVideo, sw.camera, []() {
				GUI::bImageProcRunning = true;
				GUI::bStartImageProcRequest = false;
				bImageProcStarting = false;
				PRINT_INFO("INFO: Image processing starting. DONE");
			});
		}
		else if (GUI::bImageProcRunning)
			GUI::bStartImageProcRequest = false;
	}
	else if (GUI::bStopImageProcRequest) {
		if (GUI::bImageProcRunning && !bImageProcStopping) {
			//Stop image processing
			bImageProcStopping = true;
			PRINT_INFO("INFO: Image processing stopping.");
			vidStopThread = std::thread([]() {
				vidThread.join();
				GUI::bImageProcRunning = false;
				GUI::bStopImageProcRequest = false;
				bImageProcStopping = false;
				PRINT_INFO("INFO: Image processing stopping. DONE");
			});
		}
		else if (!GUI::bImageProcRunning)
			GUI::bStopImageProcRequest = false;
	}

	if (GUI::bStartMotorContRequest) {
		if (!GUI::bMotorContRunning && !bMotorStarting) {
			//Start motor control
			bMotorStarting = true;
			PRINT_INFO("INFO: Motor connection starting.");
			motorsThread = std::thread([]() {
				motors.startup();
				GUI::bMotorContRunning = true;
				GUI::bStartMotorContRequest = false;
				bMotorStarting = false;
				PRINT_INFO("INFO: Motor connection starting. DONE");
			});
		}
		else if (GUI::bMotorContRunning)
			GUI::bStartMotorContRequest = false;
	}
	else if (GUI::bStopMotorContRequest) {
		if (GUI::bMotorContRunning && !bMotorStopping) {
			//Stop motor control
			bMotorStopping = true;
			PRINT_INFO("INFO: Motor connection terminating.");
			motorStopThread = std::thread([]() {
				motorsThread.join();
				motors.moveHome();
				delay(50);
				motors.disengage();
				GUI::bMotorContRunning = false;
				GUI::bStopMotorContRequest = false;
				bMotorStopping = false;
				PRINT_INFO("INFO: Motor connection terminating. DONE");
			});
		}
		else if (!GUI::bMotorContRunning)
			GUI::bStopMotorContRequest = false;
	}

	if (GUI::bStartSystemRequest) {
		if (!GUI::bSystemRunning && !bSystemStarting) {
			bSystemStarting = true;
			PRINT_INFO("INFO: System starting with countdown.");
			countdownThread = std::thread([]() {
				while (!GUI::bMotorContRunning || !GUI::bImageProcRunning) delay(1);
				countdownHandler();
				GUI::bSystemRunning = true;
				GUI::bStartSystemRequest = false;
				bSystemStarting = false;
				PRINT_INFO("INFO: System starting with countdown. DONE");
				delay(1000);
				PRINT_INFO("INFO: Zooming in.");
				zoom.zoomIn();
				delay(4000);
				zoom.stop();
				PRINT_INFO("INFO: Zooming in. DONE");
			});
		}
		else if (GUI::bSystemRunning)
			GUI::bStartSystemRequest = false;
	}
	else if (GUI::bStopSystemRequest) {
		if (GUI::bSystemRunning && !bSystemStopping) {
			bSystemStopping = true;
			PRINT_INFO("INFO: System stopping.");
			countdownStopThread = std::thread([]() {
				countdownThread.join();
				PRINT_INFO("INFO: Zooming out.");
				zoom.zoomOut();
				delay(4000);
				zoom.stop();
				PRINT_INFO("INFO: Zooming out. DONE");
				while (GUI::bMotorContRunning || GUI::bImageProcRunning) delay(10);
				GUI::bSystemRunning = false;
				GUI::bStopSystemRequest = false;
				bSystemStopping = false;
				PRINT_INFO("INFO: System stopping. DONE");
			});
		}
		else if (!GUI::bSystemRunning)
			GUI::bStopSystemRequest = false;
	}
}

void controlLoop() {
	static uint64_t frameCount = 0;
	PRINT_INFO("INFO: Main loop started.");
	while (!bStop) {
		stateChange();
		FindBalloonResult balPos{ 0, 0, false, false };
		if (GUI::bImageProcRunning) {
			balPos = findBalloon();
		}
		else {
			//Consume unused frames
			while (frameBuff.size()) {
				frameBuff.pop_front();
			}
		}

		if (GUI::bSystemRunning && balPos.frameReady) {
			if (balPos.noBalloon) {
				//Stop system
				GUI::bStopImageProcRequest = true;
				GUI::bStopMotorContRequest = true;
				GUI::bStopSystemRequest = true;
			}
			else {
				//Store motor rotations and update motor rotations
				auto timeMotorsSet = timeNow();
				if (frameCount == 0) {
					GUI::tMeasurementStart = timeMotorsSet;
				}
				auto& data = GUI::addData();
				data.index = frameCount;
				data.mPan = motors.getCurrentPanDegrees();
				data.mTilt = motors.getCurrentTiltDegrees();
				data.time = std::chrono::duration_cast<std::chrono::nanoseconds>(timeMotorsSet - GUI::tMeasurementStart).count() / 1E9;
				motors.addPanDegrees((balPos.x - sw.principal_x) / 10.0f);
				motors.addTiltDegrees((balPos.y - sw.principal_y) / 10.0f);
				motors.updatePos();
				frameCount++;
			}
		}
		else if (!GUI::bSystemRunning) {
			frameCount = 0;
		}
		keyboard = waitKey(1);
		if (keyboard == 'q' || keyboard == 27) {
			GUI::bStopImageProcRequest = true;
			GUI::bStopSystemRequest = true;
		}
		if (guiResult._Is_ready()) {
			GUI::bStopImageProcRequest = true;
			GUI::bStopMotorContRequest = true;
			GUI::bStopSystemRequest = true;
		}
		if (guiResult._Is_ready()
			&& !GUI::bImageProcRunning
			&& !GUI::bMotorContRunning
			&& !GUI::bSystemRunning) {
			bStop = true;
		}
	}
	PRINT_INFO("INFO: Main loop exited.");
}

int main(int argc, char* argv[]) {
	//print help information
	if (sw.print_info)
		help();
	//check for the input parameter correctness
	if (argc > 1) {
		if (sw.print_errors) {
			cerr << "Incorrect input list" << endl;
			cerr << "exiting..." << endl;
		}
		return EXIT_FAILURE;
	}

	//create GUI windows
	guiResult = std::async(GUI::StartGUI, std::ref(sw), &bStop);

	controlLoop();

	//On exit wait for reading thread
	if (vidThread.joinable())
		vidThread.join();
	if (vidStopThread.joinable())
		vidStopThread.join();
	if (motorsThread.joinable())
		motorsThread.join();
	if (motorStopThread.joinable())
		motorStopThread.join();
	if (countdownThread.joinable())
		countdownThread.join();
	if (countdownStopThread.joinable())
		countdownStopThread.join();
	guiResult.get();

	//destroy GUI windows
	destroyAllWindows();
	return EXIT_SUCCESS;
}