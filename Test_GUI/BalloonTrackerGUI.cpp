#include "SettingsWrapper.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <string>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <limits.h>
#include <thread>
#include <chrono>
#include <future>
#include <mutex>
#include <algorithm>
#include <cctype>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#ifdef _WIN64
#define _WIN32_WINNT 0x0500
#define _WIN32_IE 0x0500
#include "Windows.h"
#include <shlobj.h>
#include <stdio.h>
#pragma comment(lib, "Comdlg32")
#pragma warning(disable : 4996) //_CRT_SECURE_NO_WARNINGS
#else
#error("Program only compatible with x64 Windows")
#endif // _WIN64



namespace GUI {
	struct DataPoint {
		double x, y, z;
		double mPan, mTilt, pPan, pTilt;
		uint64_t index;
	};

	std::vector<GUI::DataPoint> data;
	std::mutex dataLock;


	__declspec(dllexport) bool bStartSystemRequest = false;
	__declspec(dllexport) bool bSystemRunning = false;
	__declspec(dllexport) bool bStopSystemRequest = false;

	__declspec(dllexport) bool bStartImageProcRequest = false;
	__declspec(dllexport) bool bImageProcRunning = false;
	__declspec(dllexport) bool bStopImageProcRequest = false;

	__declspec(dllexport) bool bStartMotorContRequest = false;
	__declspec(dllexport) bool bMotorContRunning = false;
	__declspec(dllexport) bool bStopMotorContRequest = false;


	__declspec(dllexport) double dBalloonCirc = 37.5;
	__declspec(dllexport) double dCountDown = 30;
	__declspec(dllexport) double dCountDownValue;
	__declspec(dllexport) float fBearing = 0;
}

namespace Disp {
	static std::vector<std::shared_ptr<char>> lines;
	static std::vector<char*> linesPtr;
	int iFrameCount;
	int currentLine = 0;
	std::vector<float> mPanData;
	std::vector<float> mTiltData;
	std::vector<float> pPanData;
	std::vector<float> pTiltData;
	std::vector<float> altitudeData;
}


int display_w, display_h;


using namespace GUI;



static void glfw_error_callback(int error, const char* description) {
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

std::string desktopDirectory() {
#ifdef _WIN64
	static wchar_t path[MAX_PATH + 1];

	if (SHGetFolderPath(NULL, CSIDL_DESKTOP, NULL, 0, path) != S_OK) {
		return "";
	}

	char fileName[MAX_PATH] = { '\0' };
	size_t length;
	if (wcstombs_s<MAX_PATH>(&length, fileName, path, MAX_PATH))
		return "";
	return std::string(fileName);
#else
	throw std::runtime_error("This action is not supported on non-windows platforms.");
#endif
}

static std::string getFileDialog(bool save, const wchar_t* filter, const std::string& initialDir) {
#ifdef _WIN64

	OPENFILENAME ofn;       // common dialog box structure
	wchar_t szFile[260];       // buffer for file name
	wchar_t initial[260];       // buffer for file name

	{
		size_t length;
		if (mbstowcs_s<MAX_PATH>(&length, initial, initialDir.c_str(), MAX_PATH))
			return "";
	}

	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = nullptr;
	ofn.lpstrFile = szFile;
	// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
	// use the contents of szFile to initialize itself.
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = filter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = initial;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	// Display the Open dialog box. 
	bool result;
	if (save)
		result = GetSaveFileName(&ofn) == TRUE;
	else
		result = GetOpenFileName(&ofn) == TRUE;

	if (result) {
		char fileName[512] = { '\0' };
		size_t length;
		if (wcstombs_s<512>(&length, fileName, ofn.lpstrFile, 512))
			return "";
		return std::string(fileName);
	}
	return "";
#else
	throw std::runtime_error("This action requires a file dialog; not supported on non-Windows platforms.");
#endif // _WIN64
}

static std::string getFolderDialog() {
#ifdef _WIN64
	BROWSEINFOA info;
	wchar_t wcfolderName[MAX_PATH] = { 0 };
	char titleBar[] = "Select save directory";
	ZeroMemory(&info, sizeof(info));

	info.pszDisplayName = nullptr;
	info.lpszTitle = titleBar;
	info.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;

	auto folder = SHBrowseForFolderA(&info);
	if (folder == nullptr)
		return "";

	if (!SHGetPathFromIDList(folder, wcfolderName))
		return false;

	char cfolderName[MAX_PATH] = { 0 };
	size_t length;
	if (wcstombs_s<MAX_PATH>(&length, cfolderName, wcfolderName, MAX_PATH))
		return "";
	return std::string(cfolderName);
#else
	throw std::runtime_error("This action requires a folder dialog; not supported on non-Windows platforms.");
#endif // _WIN64
}

static void saveData(std::vector<std::string> cols,
	const std::vector<double>& data, const std::string& fileName) {

	std::ofstream out;
	out.open(fileName);
	if (!out.is_open())
		return;

	size_t stride = cols.size();
	if (!stride)
		return;

	size_t size = data.size() - data.size() % stride;

	out << cols[0];
	for (size_t i = 1; i < cols.size(); i++) {
		out << "," << cols[i];
	}

	for (size_t i = 0; i < size / stride; i++) {
		out << "," << std::endl;
		out << data[i*stride];
		for (size_t j = 1; j < stride; j++) {
			out << "," << data[i*stride + j];
		}
	}
}

static void saveData(const std::string& fileName, SettingsWrapper &sw) {
	std::ofstream out;
	out.open(fileName);
	if (!out.is_open())
		return;

	if (sw.report_frame_number) {
		out << "Frame";
	}
	else {
		out << "Time";
	}
	if (sw.report_pose_estimation) {
		out << ",X,Y,Z";
	}
	if (sw.report_altitude_estimation) {
		out << ",Altitude";
	}
	if (sw.report_motor_rotation) {
		out << ",mPan,mTilt";
	}
	if (sw.report_perceived_rotation) {
		out << ",pPan,pTilt";
	}

	for (int i = 0; i < data.size(); i++) {
		std::lock_guard<std::mutex> lck(dataLock);
		out << "," << std::endl;
		if (sw.report_frame_number) {
			out << data[i].index;
		}
		else {
			out << (double) data[i].index / sw.camera_framerate;
		}
		if (sw.report_pose_estimation) {
			out << "," << data[i].x << "," << data[i].y << "," << data[i].z;
		}
		if (sw.report_altitude_estimation) {
			out << "," << data[i].y;
		}
		if (sw.report_motor_rotation) {
			out << "," << data[i].mPan << "," << data[i].mTilt;
		}
		if (sw.report_perceived_rotation) {
			out << "," << data[i].pPan << "," << data[i].pTilt;
		}
	}
}

void getTimeNowString(std::string &sFileName) {
	auto t = std::time(nullptr);
	auto tm = *std::localtime(&t);
	std::ostringstream oss;
	oss << "Session " << std::put_time(&tm, "%Y-%m-%d %H-%M-%S") << ".csv";
	sFileName = oss.str();
}

static void HelpMarker(const char* desc)
{
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

template<class T> T clamp(T val, T min, T max) {
	if (val < min)
		return min;
	if (val > max)
		return max;
	return val;
}

ImVec4 statusColour(bool req, bool run, bool reqStop) {
	ImVec4 green(0.0, 1.0, 0.0, 1.0);
	ImVec4 red(1.0, 0.0, 0.0, 1.0);
	ImVec4 yellow(1.0, 1.0, 0.0, 1.0);
	ImVec4 blue(0.0, 0.0, 1.0, 1.0);

	if (run) return green;
	if (req) return blue;
	if (reqStop) return yellow;
	return red;
}

string statusText(bool req, bool run, bool reqStop) {
	if (run) return string("Running");
	if (req) return string("Starting");
	if (reqStop) return string("Stopping");
	return string("Stopped");
}

void clearData() {
	std::lock_guard<std::mutex> lck(GUI::dataLock);

	GUI::data.clear();
	Disp::altitudeData.clear();
	Disp::currentLine = 0;
	Disp::iFrameCount = 0;
	Disp::lines.clear();
	Disp::linesPtr.clear();
	Disp::mPanData.clear();
	Disp::mTiltData.clear();
	Disp::pPanData.clear();
	Disp::pTiltData.clear();
}

//Check if the given string has the extension, else append it.
void enforceExtension(std::string& fileName, std::string&& type) {
	if (fileName.size() >= type.size()) {
		std:string ext = fileName.substr(fileName.size() - type.size());
		std::transform(ext.begin(), ext.end(), ext.begin(),
			[](unsigned char c) { return std::tolower(c); });
		std::transform(type.begin(), type.end(), type.begin(),
			[](unsigned char c) { return std::tolower(c); });
		if (ext.compare(type) != 0)
			fileName.append(type);
	}
	else {
		fileName.append(type);
	}

}














void drawSettings(SettingsWrapper& sw) {
	if (ImGui::CollapsingHeader("COM")) {
		if (bStartMotorContRequest || bMotorContRunning)
			ImGui::TextColored({ 1,0,0,1 }, "Some settings can't be changed while some systems are running.");

		int port = sw.com_port, timeout = sw.com_timeout, baud = sw.com_baud, baudIndex;
		const int baudList[] = { 110, 300, 600, 1200, 2400, 4800, 9600, 14400, 19200, 38400, 57600, 115200 };

		for (int i = 0; i < sizeof(baudList) / sizeof(int); i++) {
			if (baud == baudList[i]) {
				baudIndex = i;
				break;
			}
		}

		ImGui::Columns(3, nullptr, false);
		ImGui::Combo("Port", &port, " 0\0 1\0 2\0 3\0 4\0 5\0 6\0 7\0 8\0 9\0 10\0 11\0 12\0 13\0 14\0 15\0\0");
		ImGui::NextColumn();
		ImGui::DragInt("Timeout", &timeout, 10.0f, 0, 10000, "%d ms");
		//ImGui::InputInt("Timeout", &timeout);
		ImGui::NextColumn();
		ImGui::Combo("Baud", &baudIndex, " 110\0 300\0 600\0 1200\0 2400\0 4800\0 9600\0 14400\0 19200\0 38400\0 57600\0 115200");
		baud = baudList[baudIndex];

		if (!bStartMotorContRequest && !bMotorContRunning) {
			sw.com_port = (port = clamp(port, 0, 12));
			sw.com_timeout = (timeout = clamp(timeout, 0, 10000));
			sw.com_baud = (baud = clamp(baud, 0, 115200));
		}

	}

	ImGui::Columns(1, nullptr, false);

	if (ImGui::CollapsingHeader("Camera")) {
		if (bStartImageProcRequest || bImageProcRunning)
			ImGui::TextColored({ 1,0,0,1 }, "Some settings can't be changed while some systems are running.");
		double sensWidth = sw.sensor_width,
			sensHeight = sw.sensor_height,
			focalMin = sw.focal_length_min,
			focalMax = sw.focal_length_max,
			principalX = sw.principal_x,
			principalY = sw.principal_y;
		int imW = sw.imW,
			imH = sw.imH;

		char cameraAddress[512];
		strcpy_s<512>(cameraAddress, ((string)sw.camera).c_str());
		if (ImGui::InputText("Address", cameraAddress, 512) && !bStartImageProcRequest && !bImageProcRunning) {
			sw.camera = std::string(cameraAddress);
		}
		ImGui::SameLine(); HelpMarker("The network address of the IP camera.");

		ImGui::InputDouble("Sensor width", &sensWidth, 0.0, 0.0, "%f mm");
		ImGui::SameLine(); HelpMarker("Physical size of sensor in mm.\nOnly needs to be relative to focal length.");
		ImGui::InputDouble("Sensor height", &sensHeight, 0.0, 0.0, "%f mm");
		ImGui::SameLine(); HelpMarker("Physical size of sensor in mm.\nOnly needs to be relative to focal length.");
		ImGui::InputDouble("Minimum focal length", &focalMin, 0.0, 0.0, "%f mm");
		ImGui::SameLine(); HelpMarker("The calibrated focal length at minimum zoom.\nOnly needs to be relative to sensor size.");
		ImGui::InputDouble("Maximum focal length", &focalMax, 0.0, 0.0, "%f mm");
		ImGui::SameLine(); HelpMarker("The calibrated focal length at maximum zoom.\nOnly needs to be relative to sensor size.");
		ImGui::InputDouble("Principal point X", &principalX, 0.0, 0.0, "%f pixels");
		ImGui::SameLine(); HelpMarker("The calibrated center of the image measured in pixels.");
		ImGui::InputDouble("Principal point Y", &principalY, 0.0, 0.0, "%f pixels");
		ImGui::SameLine(); HelpMarker("The calibrated center of the image measured in pixels.");
		ImGui::InputInt("Image width", &imW);
		ImGui::SameLine(); HelpMarker("Height of images from the stream.");
		ImGui::InputInt("Image height", &imH);
		ImGui::SameLine(); HelpMarker("Height of images from the stream.");

		double inf = std::numeric_limits<double>::infinity();

		sw.sensor_width = clamp(sensWidth, 0.1, inf);
		sw.sensor_height = clamp(sensHeight, 0.1, inf);
		sw.focal_length_min = clamp(focalMin, 0.1, inf);
		sw.focal_length_max = clamp(focalMax, focalMin, inf);
		sw.principal_x = clamp(principalX, 0., inf);
		sw.principal_y = clamp(principalY, 0., inf);
		if (!bStartImageProcRequest && !bImageProcRunning) {
			sw.imW = clamp(imW, 1, INT_MAX);
			sw.imH = clamp(imH, 1, INT_MAX);
		}
	}

	if (ImGui::CollapsingHeader("Image processing")) {
		if (bStartImageProcRequest || bImageProcRunning)
			ImGui::TextColored({ 1,0,0,1 }, "Some settings can't be changed while some systems are running.");
		int RGS[3] = { (int) sw.thresh_red, (int) sw.thresh_green, (int) sw.thresh_s };
		float resizeFac = (float) sw.image_resize_factor;

		ImGui::SliderInt3("RGS threshold", RGS, 0, 255);
		ImGui::SameLine(); HelpMarker("Normalized red, green, and brightness thresholds.");
		ImGui::SliderFloat("Image resize factor", &resizeFac, 0.05f, 1.0f);
		ImGui::SameLine(); HelpMarker("Shrinking factor to improve runtime performance.");

		sw.thresh_red = clamp(RGS[0], 0, 255);
		sw.thresh_green = clamp(RGS[1], 0, 255);
		sw.thresh_s = clamp(RGS[2], 0, 255);
		if (!bStartImageProcRequest && !bImageProcRunning)
			sw.image_resize_factor = clamp(resizeFac, 0.0f, 1.0f);
	}

	if (ImGui::CollapsingHeader("Motor control")) {
		if (bStartMotorContRequest || bMotorContRunning)
			ImGui::TextColored({ 1,0,0,1 }, "Some settings can't be changed while some systems are running.");
		double panFac = sw.motor_pan_factor,
			panMin = sw.motor_pan_min,
			panMax = sw.motor_pan_max,
			panFor = sw.motor_pan_forward,
			tiltFac = sw.motor_tilt_factor,
			tiltMin = sw.motor_tilt_min,
			tiltMax = sw.motor_tilt_max,
			tiltFor = sw.motor_tilt_forward;
		int depth = sw.motor_buffer_depth;

		ImGui::InputDouble("Pan conversion factor", &panFac, 0.0, 0.0, "%f");
		ImGui::SameLine(); HelpMarker("Microseconds per degree.");
		ImGui::InputDouble("Minimum allowable tilt", &panMin, 0.0, 0.0, "%f microseconds");
		ImGui::SameLine(); HelpMarker("Safety limit in microseconds");
		ImGui::InputDouble("Maximum allowable pan", &panMax, 0.0, 0.0, "%f microseconds");
		ImGui::SameLine(); HelpMarker("Safety limit in microseconds");
		ImGui::InputDouble("Forward looking pan", &panFor, 0.0, 0.0, "%f microseconds");
		ImGui::SameLine(); HelpMarker("Microseconds to make the pan motor face forward.");
		ImGui::InputDouble("Tilt conversion factor", &tiltFac);
		ImGui::SameLine(); HelpMarker("Microseconds per degree.");
		ImGui::InputDouble("Minimum allowable tilt", &tiltMin, 0.0, 0.0, "%f microseconds");
		ImGui::SameLine(); HelpMarker("Safety limit in microseconds.");
		ImGui::InputDouble("Maximum allowable tilt", &tiltMax, 0.0, 0.0, "%f microseconds");
		ImGui::SameLine(); HelpMarker("Safety limit in microseconds.");
		ImGui::InputDouble("Forward looking tilt", &tiltFor, 0.0, 0.0, "%f microseconds");
		ImGui::SameLine(); HelpMarker("Microseconds to make the pan motor face forward.");
		ImGui::InputInt("Latency of camera", &depth);
		ImGui::SameLine(); HelpMarker("Number of frames from action to received frame.");

		if (!bStartMotorContRequest && !bMotorContRunning) {
			sw.motor_pan_factor = panFac;
			sw.motor_pan_min = panMin;
			sw.motor_pan_max = panMax;
			sw.motor_pan_forward; panFor;
			sw.motor_tilt_factor = tiltFac;
			sw.motor_tilt_min = tiltMin;
			sw.motor_tilt_max = tiltMax;
			sw.motor_tilt_forward = tiltFor;
		}
		if (!bStartImageProcRequest && !bStartMotorContRequest && !bImageProcRunning && !bMotorContRunning) {
			sw.motor_buffer_depth = depth;
		}
	}

	if (ImGui::CollapsingHeader("Display information")) {
		ImGui::Columns(1);
		ImGui::Checkbox("Show RGB image", &sw.show_frame_rgb);
		ImGui::SameLine(); HelpMarker("Show the resized input image.");
		ImGui::NextColumn();
		ImGui::Checkbox("Show masked region", &sw.show_frame_mask);
		ImGui::SameLine(); HelpMarker("Show the detected mask of the balloon.");
		ImGui::Checkbox("Show S masked region", &sw.show_frame_mask_s);
		ImGui::SameLine(); HelpMarker("Show mask from thresholded S values.");
		ImGui::Checkbox("Show R masked region", &sw.show_frame_mask_r);
		ImGui::SameLine(); HelpMarker("Show mask from thresholded R values.");
		ImGui::Checkbox("Show G masked region", &sw.show_frame_mask_g);
		ImGui::SameLine(); HelpMarker("Show mask from thresholded G values.");
		ImGui::NextColumn();
		ImGui::Checkbox("Show detection", &sw.show_frame_track);
		ImGui::SameLine(); HelpMarker("Show a shape indicating the size of the balloon.");
		ImGui::NextColumn();
		ImGui::Columns(1);
	}

	if (ImGui::CollapsingHeader("CSV generation")) {
		ImGui::Columns(1, nullptr, false);
		ImGui::Text("Choose values to be reported in resultant CSV files.");
		ImGui::Checkbox("Save raw motor rotation information", &sw.report_motor_rotation);
		ImGui::SameLine(); HelpMarker("Save the rotation of the motors for each data point.");
		ImGui::Checkbox("Save perceived balloon rotation", &sw.report_perceived_rotation);
		ImGui::SameLine(); HelpMarker("Save the compounded motor rotation and visual rotation.");
		ImGui::Checkbox("Save pose estimation", &sw.report_pose_estimation);
		ImGui::SameLine(); HelpMarker("Save the estimated (x,y,z) location of the balloon");
		ImGui::Checkbox("Save altitude estimation", &sw.report_altitude_estimation);
		ImGui::SameLine(); HelpMarker("Save the estimated altitude of the balloon");
		ImGui::Checkbox("Save frame numbers", &sw.report_pose_estimation);
		ImGui::SameLine(); HelpMarker("Each data point will have the frame number rather than\nthe closest approximation to when the frame came in.");
		ImGui::Columns(1, nullptr, false);
	}
}









void drawControls(SettingsWrapper& sw) {
	ImGui::Columns(2, nullptr, false);
	ImGui::InputDouble("Balloon circumference", &dBalloonCirc, 0, 0, "%.2f cm");
	ImGui::NextColumn();
	ImGui::InputDouble("Countdown", &dCountDown, 0, 0, "%.0f seconds");
	ImGui::NextColumn();
	ImGui::SliderFloat("Bearing", &fBearing, 0, 360, "%.1f degrees");
	ImGui::NextColumn();
	ImGui::ProgressBar((float) (dCountDownValue / dCountDown), {-1, 0},
		(std::to_string((int) (dCountDown - dCountDownValue)) + " seconds").c_str());
	ImGui::Columns(1, nullptr, false);

	ImGui::Columns(4, nullptr, false);
	ImVec4 green(0.0, 1.0, 0.0, 1.0);
	ImVec4 red(1.0, 0.0, 0.0, 1.0);
	ImVec4 yellow(1.0, 1.0, 0.0, 1.0);

	ImGui::Text("Record data");
	ImGui::NextColumn();
	ImGui::Button("        Start        ");
	ImVec2 uniformButton = ImGui::GetItemRectSize();
	if (ImGui::IsItemClicked()) {
		bStartSystemRequest = true;
		bStartImageProcRequest = true;
		bStartMotorContRequest = true;
	}
	ImGui::NextColumn();
	ImGui::Button("Stop", uniformButton);
	if (ImGui::IsItemClicked()) {
		bStopSystemRequest = true;
		bStopImageProcRequest = true;
		bStopMotorContRequest = true;
	}
	ImGui::NextColumn();
	ImGui::TextColored(statusColour(bStartSystemRequest, bSystemRunning, bStopSystemRequest) , "%7s",
		statusText(bStartSystemRequest, bSystemRunning, bStopSystemRequest).data());
	ImGui::NextColumn();

	ImGui::Text("Image Processing");
	ImGui::NextColumn();
	ImGui::Button("Start", uniformButton);
	if (ImGui::IsItemClicked()) {
		bStartImageProcRequest = true;
	}
	ImGui::NextColumn();
	ImGui::Button("Stop", uniformButton);
	if (ImGui::IsItemClicked()) {
		bStopSystemRequest = true;
		bStopImageProcRequest = true;
	}
	ImGui::NextColumn();
	ImGui::TextColored(statusColour(bStartImageProcRequest, bImageProcRunning, bStopImageProcRequest), "%7s",
		statusText(bStartImageProcRequest, bImageProcRunning, bStopImageProcRequest).data());
	ImGui::NextColumn();



	ImGui::Text("Motor Control");
	ImGui::NextColumn();
	ImGui::Button("Start", uniformButton);
	if (ImGui::IsItemClicked()) {
		bStartMotorContRequest = true;
	}
	ImGui::NextColumn();
	ImGui::Button("Stop", uniformButton);
	if (ImGui::IsItemClicked()) {
		bStopSystemRequest = true;
		bStopMotorContRequest = true;
	}
	ImGui::NextColumn();
	ImGui::TextColored(statusColour(bStartMotorContRequest, bMotorContRunning, bStopMotorContRequest), "%7s",
		statusText(bStartMotorContRequest, bMotorContRunning, bStopMotorContRequest).data());
	ImGui::NextColumn();

	ImGui::Columns(1, nullptr, false);
}







void drawRightPanel(SettingsWrapper &sw) {
	using namespace Disp;

	if (lines.size() < 1) {
		lines.emplace_back(new char[512]);
		linesPtr.emplace_back(lines[0].get());
		sprintf(lines[0].get(), "%15s %15s %15s %15s", "Frame#", "Pan", "Tilt", "Altitude");
	}
	for(size_t i = lines.size() - 1; i < data.size(); i++) {
		std::lock_guard<std::mutex> lck(dataLock);
		lines.emplace_back(new char[512]);
		char* last = lines[lines.size() - 1].get();
		sprintf(last, "%15llu %15.2f %15.2f %15.2f", data[i].index, data[i].mPan, data[i].mTilt, data[i].y);
		linesPtr.emplace_back(last);
		mPanData.push_back((float) data[i].mPan);
		mTiltData.push_back((float) data[i].mTilt);
		pPanData.push_back((float) data[i].pPan);
		pTiltData.push_back((float) data[i].pTilt);
		altitudeData.push_back((float) data[i].y);
	}

	static bool updateGraphs = true;
	static int indexA = 0, indexB = 0;
	int dataSize = altitudeData.size();
	if (updateGraphs) {
		indexB = altitudeData.size()-1;
	}


	ImGui::Columns(1);

	float gHeight = display_h * 0.1f;
	float gWidth = ImGui::GetWindowSize().x;
	ImGui::Text("Collected data:");
	ImGui::SetNextItemWidth(gWidth*0.985f);
	ImGui::ListBox("", &currentLine, linesPtr.data(), (int) lines.size(), 7);

	ImGui::Separator();

	ImGui::Text("Motor Pan (degrees):"); ImGui::SameLine(); HelpMarker("Physical rotation of the pan motor.");
	ImGui::PlotLines("", mPanData.data() + indexA, indexB - indexA, 0, "", -90, 90, { gWidth , gHeight });
	ImGui::Text("Motor Tilt (degrees):"); ImGui::SameLine(); HelpMarker("Physical rotation of the tilt motor.");
	ImGui::PlotLines("", mTiltData.data() + indexA, indexB - indexA, 0, "", -90, 90, { gWidth, gHeight });
	float maxAltitude = 0;
	if (altitudeData.size() != 0)
		maxAltitude = *std::max_element(altitudeData.begin() + indexA, altitudeData.begin() + indexB);

	ImGui::Text("Balloon Altitude (mm):"); ImGui::SameLine(); HelpMarker("Perceived altitude of the balloon.");
	ImGui::PlotLines("", altitudeData.data() + indexA, indexB - indexA, 0, "", 0, maxAltitude, { gWidth, gHeight });
	ImGui::Text("Perceived Pan (degrees):"); ImGui::SameLine();
	HelpMarker("Perceived bearing of the balloon considering all variables.");
	ImGui::PlotLines("", pPanData.data() + indexA, indexB - indexA, 0, "", -90, 90, { gWidth, gHeight });
	ImGui::Text("Perceived Tilt (degrees):"); ImGui::SameLine();
	HelpMarker("Perceived bearing of the balloon considering all variables.");
	ImGui::PlotLines("", pTiltData.data() + indexA, indexB - indexA, 0, "", -90, 90, { gWidth, gHeight });
	ImGui::Checkbox("Scrolling graphs", &updateGraphs);
	ImGui::SliderInt("Display range min", &indexA, 0, indexB);
	ImGui::SliderInt("Display range max", &indexB, indexA, dataSize-1);

	iFrameCount++;
}










namespace GUI {
	__declspec(dllexport) DataPoint& addData() {
		std::lock_guard<std::mutex> lck(dataLock);
		data.emplace_back();
		return *(data.end() - 1);
	}

	__declspec(dllexport) void addData(const GUI::DataPoint& newData) {
		std::lock_guard<std::mutex> lck(dataLock);
		data.push_back(newData);
	}



	__declspec(dllexport) int StartGUI(SettingsWrapper &sw, bool* stop) {
		if (!((std::string) sw.save_directory).size()) {
			sw.save_directory = desktopDirectory();
		}

		static std::string sFileName, sTimeFileName;


		glfwSetErrorCallback(glfw_error_callback);
		if (!glfwInit())
			return 1;

		const char* glsl_version = "#version 130";
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
		GLFWwindow* window = glfwCreateWindow(640 * 2, 720,
			"BalloonTracker", NULL, NULL);
		if (window == NULL) {
			fprintf(stderr, "Failed to open window\n");
			return 1;
		}
		glfwMakeContextCurrent(window);
		glfwSwapInterval(1);

		if (glewInit()) {
			fprintf(stderr, "Failed to initialize GLEW\n");
			return 1;
		}


		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		ImGui::StyleColorsDark();
		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init(glsl_version);



		while (!glfwWindowShouldClose(window) && !*stop) {
			glfwPollEvents();
			glfwGetFramebufferSize(window, &display_w, &display_h);
			getTimeNowString(sTimeFileName);

			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);

			if (ImGui::BeginMainMenuBar()) {
				if (ImGui::BeginMenu("File")) {
					if (ImGui::MenuItem("New data")) {
						std::string sFileResult = getFileDialog(true, L"Table (.CSV)\0*.CSV\0All\0*.*\0", sw.save_directory);
						if (sFileResult.size()) {
							sFileName = sFileResult;
							enforceExtension(sFileName, ".CSV");
							clearData();
						}
							
					}
					if (ImGui::BeginMenu("Save data")) {
						if (ImGui::MenuItem("Save As")) {
							std::string sFileResult = getFileDialog(true, L"Table (.CSV)\0*.CSV\0All\0*.*\0", sw.save_directory);
							if (sFileResult.size()) {
								sFileName = sFileResult;
								enforceExtension(sFileName, ".CSV");
								saveData(sFileName, sw);
							}
						}
						if (ImGui::MenuItem("Save")) {
							if (!sFileName.size()) {
								getTimeNowString(sTimeFileName);
								sFileName = (std::string)sw.save_directory + "\\" + sTimeFileName;
							}
							saveData(sFileName, sw);
						}
						else if (ImGui::IsItemHovered()) {

							ImGui::BeginTooltip();
							ImGui::Text(sTimeFileName.c_str());
							ImGui::EndTooltip();
						}
						ImGui::EndMenu();
					}
					if (ImGui::MenuItem("Load data")) {
						std::string sFileResult = getFileDialog(false, L"Table (.CSV)\0*.CSV\0All\0*.*\0", sw.save_directory);
						if (sFileResult.size())
							sFileName = sFileResult;
					}
					ImGui::Separator();
					if (ImGui::MenuItem("Save settings")) {
						std::string saveFile = getFileDialog(true, L"Settings (.JSON)\0*.JSON\0All\0*.*\0", sw.save_directory);
						if (saveFile.size()) {
							enforceExtension(saveFile, ".JSON");
							sw.saveSettings(saveFile);
						}
					}
					if (ImGui::MenuItem("Load settings")) {
						std::string loadFile = getFileDialog(false, L"Settings (.JSON)\0*.JSON\0All\0*.*\0", sw.save_directory);
					}
					if (ImGui::MenuItem("Select save directory")) {
						std::string sDir = getFolderDialog();
						if (sDir.size()) {
							sw.save_directory = sDir;
						}
					}
					ImGui::EndMenu();
				}
				ImGui::EndMainMenuBar();
			}

			ImGui::Begin("LeftWindow", nullptr, 0
				| ImGuiWindowFlags_NoCollapse
				| ImGuiWindowFlags_NoMove
				| ImGuiWindowFlags_NoResize
				| ImGuiWindowFlags_NoTitleBar
				| ImGuiWindowFlags_MenuBar
				| ImGuiWindowFlags_NoBringToFrontOnFocus);
			ImGui::SetWindowPos(ImVec2(0, 0));
			ImGui::SetWindowSize(ImVec2(display_w / 2.0f, (float)display_h));

			ImGui::Text("Save directory: \"%s\"", ((std::string) sw.save_directory).c_str());
			ImGui::Text("File name: \"%s\"", sFileName.size() ? sFileName.c_str() : sTimeFileName.c_str());
			ImGui::Separator();

			ImGui::Text("System controls");
			drawControls(sw);

			ImGui::Separator();

			ImGui::Text("Settings");
			drawSettings(sw);

			ImGui::End();


			ImGui::Begin("RightWindow", nullptr, 0
				| ImGuiWindowFlags_NoCollapse
				| ImGuiWindowFlags_NoMove
				| ImGuiWindowFlags_NoResize
				| ImGuiWindowFlags_NoTitleBar
				| ImGuiWindowFlags_MenuBar
				| ImGuiWindowFlags_NoBringToFrontOnFocus);
			ImGui::SetWindowPos(ImVec2((float)display_w / 2, 0));
			ImGui::SetWindowSize(ImVec2((float)display_w / 2, (float)display_h));

			drawRightPanel(sw);

			ImGui::End();

			ImGui::PopStyleVar(1);


			ImGui::Render();
			glViewport(0, 0, display_w, display_h);
			glClearColor(0.25f, 0.25f, 0.25f, 0);
			glClear(GL_COLOR_BUFFER_BIT);
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			glfwSwapBuffers(window);
		}

		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		glfwDestroyWindow(window);
		glfwTerminate();

		return 0;
	}
}

int main() {
	bool stop = false;
	SettingsWrapper sw("../settings.json");
	std::future<int> result = std::async(GUI::StartGUI, std::ref(sw), &stop);

	for (uint64_t i = 0; i < 400; i++) {
		std::lock_guard<std::mutex> lck(GUI::dataLock);
		double a = (90 * sin((i / 1) / 100.0*3.14159*2.0));
		double b = (90 * cos((i / 1) / 100.0*3.14159*2.0));
		double c = (i*i / 10000.0*(a / 180.0 + 0.5));
		data.push_back(GUI::DataPoint{ 0.0, 1.0, 2.0,
			a, b, a*b / 90.0f, -a * b / 90.0f,
			(uint64_t)i });
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}

	result.get();
}