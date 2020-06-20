#pragma once

#include <thread>
#include <vector>
#include <opencv2/core/core.hpp>

class FrameBuffer
{
public:
	std::vector<cv::Mat> frames;
	int insertIndex;
	int readIndex;
	int depth;
	bool killSignal = false;

	FrameBuffer(int depth): frames(depth), insertIndex(0), readIndex(0), depth(depth) {

	}


	void insertFrame(cv::Mat frame) {
		while (nextIndex(insertIndex) == readIndex)
			std::this_thread::yield();
		frames[insertIndex] = frame;
		insertIndex = nextIndex(insertIndex);
	}

	cv::Mat getReadFrame() {
		while (insertIndex == readIndex && !killSignal)
			std::this_thread::yield();
		if (killSignal)
			throw std::runtime_error("Attempting to read from killed buffer");
		return frames[readIndex];
	}

	void readDone() {
		frames[readIndex].release();
		readIndex = nextIndex(readIndex);
	}

	void kill() {
		killSignal = true;
	}

private:
	inline int nextIndex(int idx) {
		return (idx + 1) % depth;
	}
};

