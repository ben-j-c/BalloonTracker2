#include "VideoReader.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Secur32.lib")
#pragma comment(lib, "Bcrypt.lib")
#pragma comment(lib, "Mfplat.lib")
#pragma comment(lib, "Strmiids.lib")
#pragma comment(lib, "Mfuuid.lib")

VideoReader& VideoReader::operator=(VideoReader && other) {
	fileName = other.fileName;
	pFormatContext = other.pFormatContext;
	pLocalCodecParameters = other.pLocalCodecParameters;
	pLocalCodec = other.pLocalCodec;
	pCodecContext = other.pCodecContext;
	pPacket = other.pPacket;
	pFrame = other.pFrame;
	swsContext = other.swsContext;
	height = other.height;
	width = other.width;

	other.fileName = "";
	other.pFormatContext = nullptr;
	other.pLocalCodecParameters = nullptr;
	other.pLocalCodec = nullptr;
	other.pCodecContext = nullptr;
	other.pPacket = nullptr;
	other.pFrame = nullptr;
	other.swsContext = nullptr;

	return *this;
}

VideoReader::VideoReader(VideoReader && other) {
	*this = std::move(other);
}

VideoReader::VideoReader(const std::string & fileName) {
	this->fileName = fileName;
	try {
		openFile(fileName.c_str());
	}
	catch (std::exception&) {
		closeContext();
	}
}

VideoReader::VideoReader(const char* fileName) {
	this->fileName = fileName;
	try {
		openFile(fileName);
	}
	catch (std::exception& e) {
		closeContext();
	}
}

FrameBuffer VideoReader::readFrame() {
	if ( pFormatContext && pCodecContext && pPacket ) {
		int avReadFrameResult = 0;
		while (avReadFrameResult = av_read_frame(pFormatContext, pPacket) >= 0) {
			avcodec_send_packet(pCodecContext, pPacket);
			avcodec_receive_frame(pCodecContext, pFrame);
			if (pFrame->data[0] == nullptr)
				continue;

			AVFrame* rgbFrame;
			uint8_t* rgbBuffer;
			rgbFrame = av_frame_alloc();
			int nBytes = avpicture_get_size(AV_PIX_FMT_RGB24, pCodecContext->width, pCodecContext->height);
			rgbBuffer = (uint8_t*)av_malloc(nBytes * sizeof(uint8_t));
			avpicture_fill((AVPicture*)rgbFrame, rgbBuffer, AV_PIX_FMT_RGB24, pCodecContext->width, pCodecContext->height);

			sws_scale(swsContext, pFrame->data, pFrame->linesize, 0, pFrame->height, rgbFrame->data, rgbFrame->linesize);
			av_frame_free(&rgbFrame);
			return std::shared_ptr<uint8_t>(rgbBuffer,
				[](uint8_t* buff) { av_free(buff);});
		}

		if (avReadFrameResult < 0)
			closeContext();
	}

	return nullptr;
}

bool VideoReader::readFrame(FrameBuffer& rgbBuffer) {
	if (pFormatContext && pCodecContext && pPacket) {
		if (!rgbBuffer) {
			int nBytes = avpicture_get_size(AV_PIX_FMT_RGB24, pCodecContext->width, pCodecContext->height);
			rgbBuffer = FrameBuffer((uint8_t*)av_malloc(nBytes * sizeof(uint8_t)), [](uint8_t* buff) { av_free(buff); });
		}
		int avReadFrameResult = 0;
		while (avReadFrameResult = av_read_frame(pFormatContext, pPacket) >= 0) {
			avcodec_send_packet(pCodecContext, pPacket);
			avcodec_receive_frame(pCodecContext, pFrame);
			if (pFrame->data[0] == nullptr)
				continue;

			AVFrame* rgbFrame;
			rgbFrame = av_frame_alloc();
			int nBytes = avpicture_get_size(AV_PIX_FMT_RGB24, pCodecContext->width, pCodecContext->height);
			avpicture_fill((AVPicture*)rgbFrame, rgbBuffer.get(), AV_PIX_FMT_RGB24, pCodecContext->width, pCodecContext->height);

			sws_scale(swsContext, pFrame->data, pFrame->linesize, 0, pFrame->height, rgbFrame->data, rgbFrame->linesize);
			av_frame_free(&rgbFrame);

			return true;
		}
		if (avReadFrameResult < 0)
			closeContext();
	}

	return false;
}

int VideoReader::size() const {
	if(pCodecContext)
		return avpicture_get_size(AV_PIX_FMT_RGB24, pCodecContext->width, pCodecContext->height);
	return 0;
}

bool VideoReader::ready() const {
	return pFormatContext
		&& pLocalCodecParameters
		&& pLocalCodec
		&& pCodecContext
		&& pPacket
		&& pFrame
		&& swsContext;
}

void VideoReader::openFile(const char* url) {
	if (avformat_open_input(&pFormatContext, url, nullptr, nullptr))
		throw std::runtime_error("avformat could not open");

	if (avformat_find_stream_info(pFormatContext, nullptr) < 0) {
		throw std::runtime_error("avformat_find_stream_info returned value less than 0");
	}

	av_dump_format(pFormatContext, 0, url, 0);

	int videoStream = -1;
	for (int i = 0; i < pFormatContext->nb_streams; i++) {
		if (pFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoStream = i;
			break;
		}
	}
	if (videoStream == -1)
		throw std::runtime_error("No video stream of codec type AVMEDIA_TYPE_VIDEO");

	pLocalCodecParameters = pFormatContext->streams[videoStream]->codecpar;
	pLocalCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);

	pCodecContext = avcodec_alloc_context3(pLocalCodec);
	avcodec_parameters_to_context(pCodecContext, pLocalCodecParameters);
	avcodec_open2(pCodecContext, pLocalCodec, NULL);

	pPacket = av_packet_alloc();
	pFrame = av_frame_alloc();

	swsContext = sws_getContext(
		pCodecContext->width,
		pCodecContext->height,
		AV_PIX_FMT_YUVJ420P, pCodecContext->width, pCodecContext->height,
		AV_PIX_FMT_RGB24, 0, 0, 0, 0);

	width = pCodecContext->width;
	height = pCodecContext->height;
}

void VideoReader::closeContext() {
	if(pFrame)
		av_frame_free(&pFrame);
	if(pPacket)
		av_free_packet(pPacket);
	if(pCodecContext)
		avcodec_free_context(&pCodecContext);
	if (pFormatContext)
		avformat_free_context(pFormatContext);

	pFrame = nullptr;
	pPacket = nullptr;
	pCodecContext = nullptr;
	pFormatContext = nullptr;
	pLocalCodecParameters = nullptr;
	pLocalCodec = nullptr;
}
