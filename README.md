# Balloon Tracker
This repo is the code for the Balloon Tracker project.
The solution directory contains multiple project directories with various functions.
The core project is 'BalloonTracker' along with 'ui' for the displaying data within MATLAB.

# Dependencies
Compilation dependencies:
1. Windows 7, 8, 10
2. VisualStudio 2017
3. OpenCV 3.4.9 with CUDA

Standalone binaries dependencies:
1. VisualStudio 2017 redistributable

GUI dependencies:
1. MATLAB 2019b

# Installing
Once the dependencies have been installed, this repo can be cloned and the solution opened.
Each project requires an environment variable to locate the root directory of your OpenCV install.
The enmvironment variable should be `BT_OCV` and point to the root directory such that the following
resolve to your include and library folders: `$(BT_OCV)/include` and `$(BT_OCV)/x64/vc16/lib`

# Information About Other Projects
Even though only the `BalloonTracker` folder contains the final product, there are additional
projects that can be used for calibration or debugging. The following list details what each project is
used for:
- `IPCameraTest` : Checking the camera is accessible.
- `Test_Camera_Latency` : measuring the delay between when actions occur to when the frame is processed.
- `Test_FeedbackLoop` : Checking if the balloon is correctly placed in the world.
- `Test_FeedbackLoop_Full` : Additionally checks if the motors are correctly rotating.
- `Test_Network_1Hour` : Sends an hours worth of data ensuring the GUI can handle it.
- `Test_Network_Communication` : Sends a single packet to check data transmission.
- `Test_PanTilt_10Seconds` : Moves the pan-tilt for 10 seconds in a square pattern.
- `Test_PanTilt_1Hour` : Moves the pan-tilt for 3600 seconds in a square pattern.
- `Test_PanTilt_Angular` : Provides tool for checking angular control of the motors.
- `Test_PanTilt_Camera_Coord` : Provides tool for checking the coordination between the camera and the motors.
- `Test_PanTilt_Connection` : Attempts to make contact with the Arduino.
- `Test_PanTilt_video` : Displays the video feed while allowing control of the motors.
Used to check stability in the image during movement.
