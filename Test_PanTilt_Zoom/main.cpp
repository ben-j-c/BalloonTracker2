#define GLEW_STATIC
#define _USE_MATH_DEFINES
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "SettingsWrapper.h"
#include <ZoomHandler.h>
#include <VideoReader.h>
#include <MotorHandler.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <iostream>


SettingsWrapper sw("../settings2.1.json");
ZoomHandler zm(sw.onvifEndpoint);
MotorHandler motors(sw);
int display_w, display_h;

static void glfw_error_callback(int error, const char* description) {
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

void  setTextureData(std::shared_ptr<GLuint>& image_texture, std::shared_ptr<uint8_t>& data, int width, int height) {
	if (!image_texture)
		image_texture = std::shared_ptr<GLuint>(new GLuint, [](GLuint* p) { glDeleteTextures(1, p); delete p; });
	if (image_texture == 0) {
		glGenTextures(1, image_texture.get());
	}

	glBindTexture(GL_TEXTURE_2D, *image_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data.get());
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
	std::cout << "INFO: processVideo consuming frames." << std::endl;
	while (dt.count() < 1000.0f / 30) {
		//std::this_thread::sleep_for(milisec(10));
		auto start = timer.now();
		vid.readFrame(buff);
		auto stop = timer.now();
		dt = std::chrono::duration_cast<milisec>(stop - start);
	}
	std::cout << "INFO: processVideo consuming frames. DONE" << std::endl;
}

int main() {
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		return 1;

	const char* glsl_version = "#version 130";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

	GLFWwindow* window = glfwCreateWindow(1280, 720, "Camera Zoom Test", NULL, NULL);
	if (window == NULL)
		return 1;
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize OpenGL loader!\n");
		return 1;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	bool motorsGood = false;
	if (motorsGood = motors.startup()) {
		std::cout << "INFO: Motors good." << std::endl;
	}
	else {
		std::cout << "ERROR: Motors can't be engaged." << std::endl;
		return -1;
	}

	VideoReader vid(sw.camera);
	ImageRes buff = vid.readFrame();
	consumeBufferedFrames(vid, buff);	

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);

		////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////

		ImGui::Begin("left", nullptr, 0
			| ImGuiWindowFlags_NoCollapse
			| ImGuiWindowFlags_NoMove
			| ImGuiWindowFlags_NoResize
			| ImGuiWindowFlags_NoTitleBar
			| ImGuiWindowFlags_MenuBar
			| ImGuiWindowFlags_NoBringToFrontOnFocus);
		ImGui::SetWindowPos(ImVec2(0, 0));
		ImGui::SetWindowSize(ImVec2((float)display_w, (float)display_h));
		{
			if (vid.readFrame(buff) < 0) {
				vid = VideoReader(sw.camera);
				consumeBufferedFrames(vid, buff);
			}

			static std::shared_ptr<GLuint> texture;
			setTextureData(texture, buff, vid.getWidth(), vid.getHeight());;

			ImGui::Button("Zoom In");
			if (ImGui::IsItemHovered() && ImGui::IsMouseDown(0)) {
				zm.zoomIn();
			}
			else if (zm.zoomingIn) {
				zm.stop();
			}

			ImGui::SameLine();
			ImGui::Button("Zoom Out");
			if (ImGui::IsItemHovered() && ImGui::IsMouseDown(0)) {
				zm.zoomOut();
			}
			else if (zm.zoomingOut) {
				zm.stop();
			}

			static float fLastPanDeg = 0.0f, fLastTiltDeg = 0.0f;
			static float fPanDeg = 0.0f;
			ImGui::SliderFloat("Pan", &fPanDeg,
				(sw.motor_pan_min - sw.motor_pan_forward) / sw.motor_pan_factor,
				(sw.motor_pan_max - sw.motor_pan_forward) / sw.motor_pan_factor);
			static float fTiltDeg = 0.0f;
			ImGui::SliderFloat("Tilt", &fTiltDeg,
				(sw.motor_tilt_min - sw.motor_tilt_forward) / sw.motor_tilt_factor,
				(sw.motor_tilt_max - sw.motor_tilt_forward) / sw.motor_tilt_factor);
			ImGui::Text("%f", fPanDeg);
			ImGui::Text("%f", fTiltDeg);
			if (motorsGood && (fLastPanDeg != fPanDeg || fLastTiltDeg != fTiltDeg)) {
				motors.setNextPanDegrees(fPanDeg);
				motors.setNextTiltDegrees(fTiltDeg);
				motors.updatePos();
			}

			fLastPanDeg = fPanDeg;
			fLastTiltDeg = fTiltDeg;

			ImGui::Image((void*)(intptr_t)*texture, ImVec2(vid.getWidth() / 4, vid.getHeight() / 4));

		}
		ImGui::End();

		/*
		ImGui::Begin("right", nullptr, 0
			| ImGuiWindowFlags_NoCollapse
			| ImGuiWindowFlags_NoMove
			| ImGuiWindowFlags_NoResize
			| ImGuiWindowFlags_NoTitleBar
			| ImGuiWindowFlags_MenuBar
			| ImGuiWindowFlags_NoBringToFrontOnFocus);
		ImGui::SetWindowPos(ImVec2((float)display_w / 2.0f, 0));
		ImGui::SetWindowSize(ImVec2((float)display_w / 2.0f, (float)display_h));
		{
		}
		ImGui::End();
		*/

		ImGui::PopStyleVar(1);
		ImGui::Render();
		glClearColor(0.2, 0.2, 0.75, 0);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}

	if (motorsGood) {
		motors.moveHome();
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		motors.disengage();
	}
}
