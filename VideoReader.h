#pragma once

#include <string>
#include <memory>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

#include <opencv2/core/core.hpp>

class ImageRes {
public:
	ImageRes() : res(nullptr), width(0), height(0) {};
	ImageRes(uint8_t* ptr, int h, int w) : res(ptr, deleter), width(w), height(h) {};
	operator bool() const { return (bool) res; };
	uint8_t* get() const {
		return res.get();
	}

	operator cv::Mat() const {
		return cv::Mat(height, width, CV_8UC3, res.get());
	};

	operator std::shared_ptr<uint8_t>() const {
		return res;
	};

	operator std::shared_ptr<uint8_t>&(){
		return res;
	};
private:
	int width;
	int height;
	std::shared_ptr<uint8_t> res;

	static void deleter(uint8_t* p) {
		if (p) {
			av_free(p);
		}			
	}
};

class VideoReader {
public:
	VideoReader() = default;
	VideoReader& operator=(VideoReader&& other);
	VideoReader(VideoReader&& other);
	VideoReader(const std::string& fileName);
	VideoReader(const char* fileName);
	VideoReader(const VideoReader&&) = delete;
	VideoReader(VideoReader&) = delete;
	~VideoReader() { closeContext(); };

	ImageRes readFrame();
	int readFrame(ImageRes&);


	int size() const;
	bool ready() const;
	int getHeight() const { return height; };
	int getWidth() const { return width; };
	operator bool() const { return ready(); };
private:
	std::string fileName;
	int height = 0, width = 0 ;

	AVDictionary* options = nullptr;
	AVFormatContext *pFormatContext = nullptr;
	AVCodecParameters* pLocalCodecParameters = nullptr;
	AVCodec* pLocalCodec = nullptr;
	AVCodecContext* pCodecContext = nullptr;
	AVPacket *pPacket = nullptr;
	AVFrame *pFrame = nullptr;
	struct SwsContext* swsContext = nullptr;
	

	void openFile(const char* url);
	void closeContext();
};

