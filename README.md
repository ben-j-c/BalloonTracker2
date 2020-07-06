# Balloon Tracker
This repo is the code for the Balloon Tracker project.
The solution directory contains multiple project directories with various purposes.
The core project is 'BalloonTracker.'

# Dependencies
Compilation dependencies:
1. Windows 7, 8, 10 (I use Windows specific functions for opening files as well as things like `strcpy_s` (even though I don't need to lol)).
2. VisualStudio 2017 (don't see why you couldn't use any other version though).
3. OpenCV 3.4.9 with CUDA.

Standalone binaries dependencies:
1. [VisualStudio 2017 redistributable](https://support.microsoft.com/en-ca/help/2977003/the-latest-supported-visual-c-downloads)
2. [CUDA 10.2](https://developer.nvidia.com/cuda-10.2-download-archive)

# Build environment
Once the dependencies have been installed, this repo can be cloned and the solution opened.
Multiple environment variables are required:
- `BT_OCV`: Root directory of your OpenCV build. i.e., `$(BT_OCV)/include`, `$(BT_OCV)/x64/vc16/lib`, and `$(BT_OCV)/x64/vc15/lib` should all resolve to the proper directories (vc16 for debug binaries iirc, and vc15 for deployment binaries). Feel free to change `vcXX` to whatever suits you. I only have two different libraries because I was too lazy to recompile both debug and release versions of OpenCV.
- `GLFW_x64`: Root directory for [GLFW 3.0](https://www.glfw.org/download.html). i.e., `$(GLFW_x64)/lib` should resolve to the library and `$(GLFW_x64)/include` for the headers.
- `GLEW_PATH`: Root directory for [GLEW](http://glew.sourceforge.net/). i.e., `$(GLEW_PATH)/lib/Release/x64` and `$(GLEW_PATH)/include` blah blah.

Notice that OpenCV, GLFW, and GLEW all have DLLs. Make sure these are somewhere on your path. OpenCV specifically has the debug (`opencvworld349d.dll`) and release (`opencvworld349.dll`) dlls. I'm also pretty sure that OpenCV has ffmpeg DLLs as well (which I use), so get that on your path too.

# Information About Other Projects
Even though only the `BalloonTracker` folder contains the final product, there are additional
projects that can be used for calibration or debugging. The following list details what each project is
used for:
- `GUI`: imgui as a library.
- `IPCameraTest` : Checking the camera is accessible.
- `PTC`: Arduino code for controlling pan/tilt.
- `SerialPort`: Serial port library.
- `SettingsWrapper`: Settings handler as a library.
- `Test_Camera_Display`: Project for attempting different video acquisition methods.
- `Test_Camera_Latency` : measuring the delay between when actions occur to when the frame is processed.
- `Test_FeedbackLoop` : Checking if the balloon is correctly placed in the world.
- `Test_FeedbackLoop_Full` : Additionally checks if the motors are correctly rotating.
- `Test_GUI/BalloonTrackerGUI`: Application GUI as a DLL.
- `Test_GUI_Application`: Prototype application GUI.
- `Test_GUI_Example`: Imgui example GUI application. 
- `Test_Network_1Hour` : Sends an hours worth of data ensuring the GUI can handle it.
- `Test_Network_Communication` : Sends a single packet to check data transmission.
- `Test_PanTilt_10Seconds` : Moves the pan-tilt for 10 seconds in a square pattern.
- `Test_PanTilt_1Hour` : Moves the pan-tilt for 3600 seconds in a square pattern.
- `Test_PanTilt_Angular` : Provides tool for checking angular control of the motors.
- `Test_PanTilt_Camera_Coord` : Provides tool for checking the coordination between the camera and the motors.
- `Test_PanTilt_Connection` : Attempts to make contact with the Arduino.
- `Test_PanTilt_video` : Displays the video feed while allowing control of the motors.
- `Test_Screenshot` : Application to save screenshots from the camera.
Used to check stability in the image during movement.
