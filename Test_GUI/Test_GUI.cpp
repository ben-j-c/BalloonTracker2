#include "Test_GUI.h"
#include "SettingsWrapper.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <limits.h>

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
#endif // _WIN64

static void glfw_error_callback(int error, const char* description) {
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

std::string desktopDirectory() {
	static wchar_t path[MAX_PATH + 1];

	if (SHGetFolderPath(NULL, CSIDL_DESKTOP, NULL, 0, path) != S_OK) {
		return "";
	}

	char fileName[MAX_PATH] = { '\0' };
	size_t length;
	if (wcstombs_s<MAX_PATH>(&length, fileName, path, MAX_PATH))
		return "";
	return std::string(fileName);

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
#endif // _WIN64
}

static std::string getFolderDialog() {
#ifdef _WIN64
	BROWSEINFOA info;
	wchar_t wcfolderName[MAX_PATH] = {0};
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

#endif // _WIN64
}

static void saveData(std::vector<std::string> cols,
	const std::vector<double>& data, const std::string& fileName) {

	std::ofstream out;
	out.open(fileName);
	if (!out.is_open())
		return;

	int stride = cols.size();
	if (!stride)
		return;

	int size = data.size() - data.size() % stride;

	out << cols[0];
	for (int i = 1; i < cols.size(); i++) {
		out << "," << cols[i];
	}

	for (int i = 0; i < size/stride; i++) {
		out << "," << std::endl;
		out << data[i*stride];
		for (int j = 1; j < stride; j++) {
			out << "," << data[i*stride + j];
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

int main() {
	SettingsWrapper sw("../settings.json");
	if (!sw.save_directory.size()) {
		sw.save_directory = desktopDirectory();
	}

	static std::string sFileName, sTimeFileName;


	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		return 1;

	const char* glsl_version = "#version 130";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	GLFWwindow* window = glfwCreateWindow(640*2, 720,
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


	int display_w, display_h;
	while (!glfwWindowShouldClose(window)) {
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
					std::string sFileResult = getFileDialog(true, L"Table\0*.CSV\0All\0*.*\0", sw.save_directory);
					if (sFileResult.size())
						sFileName = sFileResult;
				}
				if (ImGui::BeginMenu("Save data")) {
					if (ImGui::MenuItem("Save As")) {
						std::string sFileResult = getFileDialog(true, L"Table\0*.CSV\0All\0*.*\0", sw.save_directory);
						if (sFileResult.size()) {
							sFileName = sFileResult;
							saveData({ "A", "B", "C" }, { 1.0,2.0,3.0,4.0,5.0,6.0 }, sFileName);
						}
					}
					if (ImGui::MenuItem("Save")) {
						if (!sFileName.size()) {
							getTimeNowString(sTimeFileName);
							sFileName = sw.save_directory + "\\" + sTimeFileName;
						}
						saveData({ "A", "B", "C" }, { 1.0,2.0,3.0,4.0,5.0,6.0 }, sFileName);
					}
					else if (ImGui::IsItemHovered()) {
						
						ImGui::BeginTooltip();
						ImGui::Text(sTimeFileName.c_str());
						ImGui::EndTooltip();
					}
					ImGui::EndMenu();
				}
				if (ImGui::MenuItem("Load data")) {
					std::string sFileResult = getFileDialog(false, L"Table\0*.CSV\0All\0*.*\0", sw.save_directory);
					if (sFileResult.size())
						sFileName = sFileResult;
				}
				ImGui::Separator();
				if (ImGui::MenuItem("Save settings")) {
					std::string saveFile = getFileDialog(true, L"Settings\0*.JSON\0All\0*.*\0", sw.save_directory);
				}
				if (ImGui::MenuItem("Load settings")) {
					std::string loadFile = getFileDialog(false, L"Settings\0*.JSON\0All\0*.*\0", sw.save_directory);
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
		ImGui::SetWindowSize(ImVec2(display_w/2, display_h));

		ImGui::Text("Save directory: \"%s\"", sw.save_directory.c_str());
		ImGui::Text("File name: \"%s\"", sFileName.size()? sFileName.c_str() : sTimeFileName.c_str());
		ImGui::Separator();

		ImGui::Text("Settings");

		if (ImGui::CollapsingHeader("COM")) {
			int port = sw.com_port, timeout = sw.com_timeout, baud = sw.com_baud;
			ImGui::Columns(3, nullptr, false);
			ImGui::InputInt("Port", &port, 1, 1);
			ImGui::NextColumn();
			ImGui::InputInt("Timeout", &timeout);
			ImGui::NextColumn();
			ImGui::InputInt("Baud", &baud);

			sw.com_port	= (port			= clamp(port	, 0, 12));
			sw.com_timeout = (timeout	= clamp(timeout	, 0, 10000));
			sw.com_baud = (baud			= clamp(baud	, 0, 115200));
			
		}
	
		ImGui::Columns(1, nullptr, false);

		if (ImGui::CollapsingHeader("Camera")) {
			double sensWidth = sw.sensor_width,
				sensHeight = sw.sensor_height,
				focalMin = sw.focal_length_min,
				focalMax = sw.focal_length_max,
				principalX = sw.principal_x,
				principalY = sw.principal_y;
			int imW = sw.imW,
				imH = sw.imH;

			char cameraAddress[512];
			strcpy_s<512>(cameraAddress, sw.camera.c_str());
			if (ImGui::InputText("Address", cameraAddress, 512)) {
				sw.camera = std::string(cameraAddress);
			}
			ImGui::SameLine(); HelpMarker("The network address of the IP camera.");

			ImGui::InputDouble("Sensor width", &sensWidth);
			ImGui::SameLine(); HelpMarker("Physical size of sensor in mm.\nOnly needs to be relative to focal length.");
			ImGui::InputDouble("Sensor height", &sensHeight);
			ImGui::SameLine(); HelpMarker("Physical size of sensor in mm.\nOnly needs to be relative to focal length.");
			ImGui::InputDouble("Minimum focal length", &focalMin);
			ImGui::SameLine(); HelpMarker("The calibrated focal length at minimum zoom.\nOnly needs to be relative to sensor size.");
			ImGui::InputDouble("Maximum focal length", &focalMax);
			ImGui::SameLine(); HelpMarker("The calibrated focal length at maximum zoom.\nOnly needs to be relative to sensor size.");
			ImGui::InputDouble("Principal point X", &principalX);
			ImGui::SameLine(); HelpMarker("The calibrated center of the image measured in pixels.");
			ImGui::InputDouble("Principal point Y", &principalY);
			ImGui::SameLine(); HelpMarker("The calibrated center of the image measured in pixels.");
			ImGui::InputInt("Image width", &imW);
			ImGui::SameLine(); HelpMarker("Height of images from the stream.");
			ImGui::InputInt("Image height", &imH);
			ImGui::SameLine(); HelpMarker("Height of images from the stream.");

			double inf = std::numeric_limits<double>::infinity();

			sw.sensor_width		= clamp(sensWidth,	0.1, inf);
			sw.sensor_height	= clamp(sensHeight, 0.1, inf);
			sw.focal_length_min = clamp(focalMin,	0.1, inf);
			sw.focal_length_max = clamp(focalMax,	focalMin, inf);
			sw.principal_x		= clamp(principalX, 0., inf);
			sw.principal_y		= clamp(principalY, 0., inf);
			sw.imW				= clamp(imW,		1, INT_MAX);
			sw.imH				= clamp(imH,		1, INT_MAX);
		}

		if (ImGui::CollapsingHeader("Image processing")) {
			int RGS[3] = { sw.thresh_red, sw.thresh_green, sw.thresh_s };
			float resizeFac = sw.image_resize_factor;

			ImGui::SliderInt3("RGS threshold", RGS, 0, 255);
			ImGui::SameLine(); HelpMarker("Normalized red, green, and brightness thresholds.");
			ImGui::SliderFloat("Image resize factor", &resizeFac, 0.05f, 1.0f);
			ImGui::SameLine(); HelpMarker("Shrinking factor to improve runtime performance.");
			
			sw.thresh_red	= clamp(RGS[0], 0, 255);
			sw.thresh_green = clamp(RGS[1], 0, 255);
			sw.thresh_s		= clamp(RGS[2], 0, 255);
			sw.image_resize_factor = clamp(resizeFac, 0.0f, 1.0f);
		}

		if (ImGui::CollapsingHeader("Motor control")) {
			double panFac = sw.motor_pan_factor,
				panMin = sw.motor_pan_min,
				panMax = sw.motor_pan_max,
				panFor = sw.motor_pan_forward,
				tiltFac = sw.motor_tilt_factor,
				tiltMin = sw.motor_tilt_min,
				tiltMax = sw.motor_tilt_max,
				tiltFor = sw.motor_tilt_forward;
			int depth = sw.motor_buffer_depth;

			ImGui::InputDouble("Pan conversion factor", &panFac);
			ImGui::SameLine(); HelpMarker("Microseconds per degree.");
			ImGui::InputDouble("Minimum allowable tilt", &panMin);
			ImGui::SameLine(); HelpMarker("Safety limit in microseconds");
			ImGui::InputDouble("Maximum allowable pan", &panMax);
			ImGui::SameLine(); HelpMarker("Safety limit in microseconds");
			ImGui::InputDouble("Forward looking pan", &panFor);
			ImGui::SameLine(); HelpMarker("Microseconds to make the pan motor face forward.");
			ImGui::InputDouble("Tilt conversion factor", &tiltFac);
			ImGui::SameLine(); HelpMarker("Microseconds per degree.");
			ImGui::InputDouble("Minimum allowable tilt", &tiltMin);
			ImGui::SameLine(); HelpMarker("Safety limit in microseconds");
			ImGui::InputDouble("Maximum allowable tilt", &tiltMax);
			ImGui::SameLine(); HelpMarker("Safety limit in microseconds");
			ImGui::InputDouble("Forward looking tilt", &tiltFor);
			ImGui::SameLine(); HelpMarker("Microseconds to make the pan motor face forward.");
			ImGui::InputInt("Latency of camera", &depth);
			ImGui::SameLine(); HelpMarker("Number of frames from action to received frame");
		}

		if (ImGui::CollapsingHeader("Display information")) {
			ImGui::Columns(3, nullptr, false);
			ImGui::Checkbox("Show RGB image", &sw.show_frame_rgb);
			ImGui::SameLine(); HelpMarker("Show the resized input image.");
			ImGui::NextColumn();
			ImGui::Checkbox("Show masked region", &sw.show_frame_mask);
			ImGui::SameLine(); HelpMarker("Show the detected mask of the balloon");
			ImGui::NextColumn();
			ImGui::Checkbox("Show detection", &sw.show_frame_track);
			ImGui::SameLine(); HelpMarker("Show a shape indicating the size of the balloon");
			ImGui::NextColumn();

			ImGui::Columns(1, nullptr, false);
		}

		ImGui::End();


		ImGui::Begin("RightWindow", nullptr, 0
			| ImGuiWindowFlags_NoCollapse
			| ImGuiWindowFlags_NoMove
			| ImGuiWindowFlags_NoResize
			| ImGuiWindowFlags_NoTitleBar
			| ImGuiWindowFlags_MenuBar
			| ImGuiWindowFlags_NoBringToFrontOnFocus);
		ImGui::SetWindowPos(ImVec2(display_w/2, 0));
		ImGui::SetWindowSize(ImVec2(display_w/2, display_h));

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
