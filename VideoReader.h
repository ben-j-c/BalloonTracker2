#pragma once

#include <string>
#include <memory>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

typedef std::shared_ptr<uint8_t> FrameBuffer;

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

	FrameBuffer readFrame();
	bool readFrame(FrameBuffer&);


	int size() const;
	bool ready() const;
	int getHeight() const { return height; };
	int getWidth() const { return width; };
	operator bool() const { return ready(); };
private:
	std::string fileName;
	int height = 0, width = 0 ;

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

