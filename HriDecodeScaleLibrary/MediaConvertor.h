#pragma once

#include <queue>
#include <thread>

using namespace std;

extern "C"
{
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/file.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
};

typedef struct buffer_data {
	uint8_t *ptr;
	size_t size; ///< size left in the buffer
}BufData;

class MediaConvertor
{
public:
	MediaConvertor();
	~MediaConvertor();

public:
	MediaConvertor(int fmt, int width, int height);
	void initParam();
	void releaseParam();
	int manageFrame(int playType, uint8_t *src, int len, uint8_t *out, int &oLen, float &fps);
	
private:
	bool findMediaInfo(uint8_t *inBuf, uint64_t len);
	

private:
	enum AVPixelFormat pixFmtOut, pixFmtIn;
	AVCodecContext *video_dec_ctx;

private:
	string verName;
	int widthOut, heightOut;
	float fps;
	uint32_t recFrames;
	uint64_t recBytes;
	uint8_t *frameBuf;
	bool readyDecoder;
	queue<BufData> *videoQueue;
	queue<BufData> *imageQueue;
	bool decodeFlag;
	thread *decodeThread;
};

