# Balloon Tracker
This repo is the code for the Balloon Tracker project. The solution directory contains multiple project directories with various functions.

The core project is `BalloonTracker`. The core project does not have a GUI, so the GUI project is also needed to run.
The GUI for balloon tracker can be seen in the `ui` directory. The GUI runs from within MATLAB.

# Dependencies
The projects included require software to compile:
1. Windows 7, 8, 10
2. VisualStudio 2017
3. OpenCV 3.4.9 with CUDA

The standalone binaries have other dependencies:
1. VisualStudio 2017 redistributable

The GUI script has additional dependencies:
1. MATLAB 2019b

# Installing
Once the dependencies have been installed, this repo can be cloned and the solution opened.
Each project requires an environment variable to locate the root directory of your OpenCV install.
The enmvironment variable should be `BT_OCV` and point to the root directory such that the following
resolve to your include and library folders: `$(BT_OCV)/include` and `$(BT_OCV)/x64/vc16/lib`