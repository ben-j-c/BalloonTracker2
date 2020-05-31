#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>

#include "SettingsWrapper.h"

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"


#include "Test_GUI.h"
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

	if (SHGetFolderPath(NULL, CSIDL_DESKTOP, NULL, 0, path) == S_OK) {
		MessageBox(NULL, path, L"TEST", MB_OK); //test
	}
	else {
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
	HWND hwnd;              // owner window
	HANDLE hf;              // file handle


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
	oss << "Balloon Tracker " << std::put_time(&tm, "%d-%m-%Y %H-%M-%S") << ".csv";
	sFileName = oss.str();
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
	GLFWwindow* window = glfwCreateWindow(640, 720,
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

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);


		ImGui::Begin("LeftWindow", nullptr, 0
			| ImGuiWindowFlags_NoCollapse
			| ImGuiWindowFlags_NoMove
			| ImGuiWindowFlags_NoResize
			| ImGuiWindowFlags_NoTitleBar
			| ImGuiWindowFlags_MenuBar
			| ImGuiWindowFlags_NoBringToFrontOnFocus);
		ImGui::SetWindowPos(ImVec2(0, 0));
		ImGui::SetWindowSize(ImVec2(display_w/2, display_h));

		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("New data")) {
					sFileName = getFileDialog(true, L"All\0*.*\0Table\0*.CSV\0", sw.save_directory);
				}
				if (ImGui::BeginMenu("Save data")) {
					if (ImGui::MenuItem("Save As")) {
						sFileName = getFileDialog(true, L"All\0*.*\0Table\0*.CSV\0", sw.save_directory);
					}
					if (ImGui::MenuItem("Save")) {
						getTimeNowString(sTimeFileName);
						sFileName = sw.save_directory + "\\" + sTimeFileName;
						saveData({ "A", "B", "C" }, { 1.0,2.0,3.0,4.0,5.0,6.0 }, sFileName);
					}
					else if (ImGui::IsItemHovered()) {
						getTimeNowString(sTimeFileName);
						ImGui::BeginTooltip();
						ImGui::Text(sTimeFileName.c_str());
						ImGui::EndTooltip();
					}
					ImGui::EndMenu();
				}
				if (ImGui::MenuItem("Load data")) {
					sFileName = getFileDialog(false, L"All\0*.*\0Table\0*.CSV\0", sw.save_directory);
				}
				ImGui::Separator();
				if (ImGui::MenuItem("Save settings")) {
					std::string saveFile = getFileDialog(true, L"All\0*.*\0Settings\0*.JSON\0", sw.save_directory);
				}
				if (ImGui::MenuItem("Load settings")) {
					std::string loadFile = getFileDialog(false, L"All\0*.*\0Settings\0*.JSON\0", sw.save_directory);
				}
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
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
