#include "MediaConvertor.h"

#include <string>
#include <thread>

enum PlayType
{
	ONLINE = 1,
	RECORD = 2,
	IMAGE = 3
};

static const int VIDEOFRAMELEN = 80;
static const int IMAGEFRAMELEN = 40;
static const int FRAMEBUFLEN = 500 * 1024;

int read_packet(void * opaque, uint8_t * buf, int buf_size)
{
	struct buffer_data *bd = (struct buffer_data *)opaque;
	buf_size = FFMIN(buf_size, bd->size);

	if (!buf_size)
		return AVERROR_EOF;
	printf("ptr:%p size:%zu\n", bd->ptr, bd->size);

	/* copy internal buffer data to buf */
	memcpy(buf, bd->ptr, buf_size);
	bd->ptr += buf_size;
	bd->size -= buf_size;

	return buf_size;
}

int open_codec_context(int *stream_idx,
	AVCodecContext **dec_ctx, AVFormatContext *fmt_ctx, enum AVMediaType type)
{
	int ret, stream_index;
	AVStream *st;
	AVCodec *dec = NULL;
	AVDictionary *opts = NULL;

	ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
	if (ret < 0) {
		fprintf(stderr, "Could not find %s stream in input file '%s'\n",
			av_get_media_type_string(type), "filename");
		return ret;
	}
	else {
		stream_index = ret;
		st = fmt_ctx->streams[stream_index];

		/* find decoder for the stream */
		dec = avcodec_find_decoder(st->codecpar->codec_id);
		if (!dec) {
			fprintf(stderr, "Failed to find %s codec\n",
				av_get_media_type_string(type));
			return AVERROR(EINVAL);
		}

		/* Allocate a codec context for the decoder */
		*dec_ctx = avcodec_alloc_context3(dec);
		if (!*dec_ctx) {
			fprintf(stderr, "Failed to allocate the %s codec context\n",
				av_get_media_type_string(type));
			return AVERROR(ENOMEM);
		}

		/* Copy codec parameters from input stream to output codec context */
		if ((ret = avcodec_parameters_to_context(*dec_ctx, st->codecpar)) < 0) {
			fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n",
				av_get_media_type_string(type));
			return ret;
		}

		/* Init the decoders, with or without reference counting */
		av_dict_set(&opts, "refcounted_frames", 1 ? "1" : "0", 0);
		if ((ret = avcodec_open2(*dec_ctx, dec, &opts)) < 0) {
			fprintf(stderr, "Failed to open %s codec\n",
				av_get_media_type_string(type));
			return ret;
		}
		*stream_idx = stream_index;
	}

	return 0;
}

int decode2(AVCodecContext *dec_ctx, AVFrame *frame, AVPacket *pkt)
{
	int got_frame = 0;
	int ret = avcodec_decode_video2(dec_ctx, frame, &got_frame, pkt);
	if (got_frame)
	{
		return 0;
	}
	return -1;
}

int decode(AVCodecContext *dec_ctx, AVFrame *frame, AVPacket *pkt)
{
	int ret;

	ret = avcodec_send_packet(dec_ctx, pkt);
	if (ret < 0) {
		fprintf(stderr, "Error sending a packet for decoding\n");
		return ret;
	}
	printf("avcodec_send_packet ret=%d\n", ret);
	while (ret >= 0) {
		ret = avcodec_receive_frame(dec_ctx, frame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			return ret;
		else if (ret < 0) {
			fprintf(stderr, "Error during decoding\n");
			return ret;
		}

		printf("saving frame %3d\n", dec_ctx->frame_number);
		fflush(stdout);
		return ret;
	}
}


void decodeScaleThread(queue<BufData> *in, queue<BufData> *out, bool *flag, 
	AVCodecContext *c, int oW, int oH, AVPixelFormat oFmt, float *ifps)
{
	printf("decodeScaleThread entre :%d codec id:%d\n", this_thread::get_id(), c->codec_id);
	AVPacket *pkt = av_packet_alloc();
	AVCodecParserContext *parser = av_parser_init(c->codec_id);
	AVFrame *frame = av_frame_alloc();
	struct SwsContext *sws_ctx = sws_getContext(c->width, c->height, c->pix_fmt,
		oW, oH, oFmt, SWS_BILINEAR, NULL, NULL, NULL);
	uint8_t *dst_data[4];
	int dst_linesize[4], dst_bufsize;
	int ret = av_image_alloc(dst_data, dst_linesize, oW, oH, oFmt, 1);
	dst_bufsize = ret;
	
	do{
		int iSize = in->size();
		printf("in size = %d\n", iSize);
		if (iSize > 0)
		{
			BufData bd = in->front();
			in->pop();
			uint8_t *data = bd.ptr;
			int data_size = bd.size;
			printf("datasize=%d data[6]=%d\n", data_size, data[6]);
			while (data_size > 0)
			{
				ret = av_parser_parse2(parser, c, &pkt->data, &pkt->size,
					data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
				if (ret < 0) {
					fprintf(stderr, "Error while parsing\n");
				}
				data += ret;
				data_size -= ret;
				printf("ret = %d pkt->size = %d datasize=%d\n", ret, pkt->size, data_size);
				if (pkt->size) {
					int retDec = decode(c, frame, pkt);
					//int retDec = decode2(c, frame, pkt);
					printf("retDec = %d\n", retDec);
					if (retDec == 0)
					{
						sws_scale(sws_ctx, (const uint8_t * const*)(frame->data),
							frame->linesize, 0, frame->height, dst_data, dst_linesize);
						BufData image;
						image.ptr = new uint8_t[dst_bufsize];
						image.size = dst_bufsize;
						memcpy(image.ptr, dst_data[0], dst_bufsize);
						out->push(image);
						float dfps = (float)av_q2d(c->framerate);
						if (dfps>0 && dfps<50 && dfps!=*ifps)
						{
							printf("dfps = %f fps = %f\n", dfps, *ifps);
							*ifps = dfps;	
						}
						printf("success finish frame %3d fps=%f duration=%ld\n", 
							c->frame_number, *ifps, pkt->duration);
					}
				}
			}
			delete bd.ptr;
		}
		else
		{
			this_thread::sleep_for(chrono::milliseconds(50));
		}
		int oSize = out->size();
		printf("out size = %d\n", oSize);
	} while (*flag);

	av_parser_close(parser);
	av_frame_free(&frame);
	av_packet_free(&pkt);
	sws_freeContext(sws_ctx);
	printf("decodeScaleThread exit : %d\n", this_thread::get_id());
}

MediaConvertor::MediaConvertor()
{
}


MediaConvertor::~MediaConvertor()
{
	printf("~MediaConvertor\n");
	releaseParam();
}

MediaConvertor::MediaConvertor(int fmt, int width, int height)
{
	verName = "0.0.1";
	widthOut = width;
	heightOut = height;
	switch (fmt)
	{
	case 1:
		pixFmtOut = AV_PIX_FMT_BGR24;
		break;
	case 2:
		pixFmtOut = AV_PIX_FMT_RGB565LE;
		break;
	case 3:
		pixFmtOut = AV_PIX_FMT_BGRA;
		break;
	default:
		break;
	}
	printf("MediaConvertor version:%s fmt:%s width=%d height=%d\n", 
		verName.c_str(), av_get_pix_fmt_name(pixFmtOut), widthOut, heightOut);
	initParam();
}

void MediaConvertor::initParam()
{
	frameBuf = new uint8_t[FRAMEBUFLEN];
	recFrames = 0;
	recBytes = 0;
	readyDecoder = false;
	decodeFlag = true;
	videoQueue = new queue<BufData>;
	imageQueue = new queue<BufData>;
	decodeThread = NULL;
	//
	video_dec_ctx = NULL;
}

void MediaConvertor::releaseParam()
{
	printf("releaseParam:%d\n", decodeFlag);
	decodeFlag = false;
	if (decodeThread != NULL)
	{
		decodeThread->join();
	}
	delete frameBuf;
	avcodec_free_context(&video_dec_ctx);
	printf("releaseParam:%d\n", decodeFlag);
}

int MediaConvertor::manageFrame(int playType, uint8_t * src, int iLen, uint8_t * out, int & oLen, float &ifps)
{
	int ret;
	
	if (iLen > 0)
	{
		BufData buf;
		buf.ptr = new uint8_t[iLen];
		buf.size = iLen;
		memcpy(buf.ptr, src, buf.size);
		videoQueue->push(buf);
		//printf("videoQueue size = %d src[6]=%d\n", videoQueue->size(), src[6]);
	}

	if(!readyDecoder)
	{
		if ((recBytes+iLen) > FRAMEBUFLEN)
		{
			recBytes = 0;
		}
		memcpy(frameBuf + recBytes, src, iLen);
	}
	if (iLen > 0)
	{
		recBytes += iLen;
		recFrames += 1;
		printf("playType =%d received data=%d frames=%ld bytes=%lld finded=%s data[6]=%d\n",
			playType, iLen, recFrames, recBytes, readyDecoder == true ? "true" : "false", frameBuf[6]);
	}
	
	if (!readyDecoder && recFrames%6==0)
	{
		printf("start find decoder\n");
		readyDecoder = findMediaInfo(frameBuf, recBytes);
		//
		if (readyDecoder)
		{
			if (playType == ONLINE)
			{
				while (!videoQueue->empty())
				{
					delete videoQueue->front().ptr;
					videoQueue->pop();
				}
				decodeThread = new thread(decodeScaleThread, videoQueue, imageQueue, &decodeFlag,
					video_dec_ctx, widthOut, heightOut, pixFmtOut, &ifps);
			}
			else if (playType == RECORD)
			{
				decodeThread = new thread(decodeScaleThread, videoQueue, imageQueue, &decodeFlag,
					video_dec_ctx, widthOut, heightOut, pixFmtOut, &ifps);
			}
			else if(playType == IMAGE)
			{
				while (!videoQueue->empty())
				{
					delete videoQueue->front().ptr;
					videoQueue->pop();
				}
				BufData bufd = { 0 };
				bufd.ptr = new uint8_t[recBytes];
				bufd.size = recBytes;
				memcpy(bufd.ptr, frameBuf, bufd.size);
				videoQueue->push(bufd);
				decodeFlag = false;
				decodeScaleThread(videoQueue, imageQueue, &decodeFlag,
					video_dec_ctx, widthOut, heightOut, pixFmtOut, &ifps);
			}
			ifps = fps;
		}
	}
	int is = imageQueue->size();
	printf("imagequeue size = %d\n", is);
	if (is > 0)
	{
		BufData image = imageQueue->front();
		memcpy(out, image.ptr, image.size);
		oLen = image.size;
		imageQueue->pop();
		delete image.ptr;
		return is;
	}

	return -1;
}

/*
* 寻找媒体信息及解码器
*/
bool MediaConvertor::findMediaInfo(uint8_t *inBuf, uint64_t len)
{
	printf("findMediaInfo len=%d data[6]=%d\n", len, *(uint8_t *)(inBuf+6));
	AVFormatContext *fmt_ctx = NULL;
	AVIOContext *avio_ctx = NULL;
	uint8_t *avio_ctx_buffer = NULL;
	size_t avio_ctx_buffer_size = 4096;
	struct buffer_data bd = { 0 };
	int ret = 0;

	if (!(fmt_ctx = avformat_alloc_context())) {
		ret = AVERROR(ENOMEM);
		goto end;
	}

	bd.ptr = inBuf;
	bd.size = (size_t)len;
	avio_ctx_buffer = (uint8_t*)av_malloc(avio_ctx_buffer_size);
	if (!avio_ctx_buffer) {
		printf("avio_ctx_buffer error\n");
		goto end;
	}
	avio_ctx = avio_alloc_context(avio_ctx_buffer, avio_ctx_buffer_size,
		0, &bd, &read_packet, NULL, NULL);
	if (!avio_ctx) {
		printf("avio_ctx error\n");
		goto end;
	}

	fmt_ctx->pb = avio_ctx;
	ret = avformat_open_input(&fmt_ctx, NULL, NULL, NULL);
	if (ret < 0) {
		printf("avformat_open_input error\n");
		goto end;
	}
	ret = avformat_find_stream_info(fmt_ctx, NULL);
	if (ret < 0) {
		printf("avformat_find_stream_info error\n");
		goto end;
	}
	av_dump_format(fmt_ctx, 0, NULL, 0);

	//找到解码器-初始化解码器参数
	int video_stream_idx = -1;
	if (open_codec_context(&video_stream_idx, &video_dec_ctx, fmt_ctx, AVMEDIA_TYPE_VIDEO) >= 0) {

		int win = video_dec_ctx->width;
		int hin = video_dec_ctx->height;
		pixFmtIn = video_dec_ctx->pix_fmt;
		float fpsFmt = av_q2d(fmt_ctx->streams[video_stream_idx]->r_frame_rate);
		if (fpsFmt>0 && fpsFmt<50)
		{
			fps = fpsFmt;
			printf("fpsFmt = %f\n", fpsFmt);
		}
		printf("in width=%d height=%d pix=%s fps=%f\n", win, hin, av_get_pix_fmt_name(pixFmtIn), fps);
		if (widthOut==0 || heightOut==0)
		{
			widthOut = win;
			heightOut = hin;
		}
		printf("out width=%d height=%d pix=%s\n", widthOut, heightOut, av_get_pix_fmt_name(pixFmtOut));
	}

end:
	avformat_close_input(&fmt_ctx);
	/* note: the internal buffer could have changed, and be != avio_ctx_buffer */
	if (avio_ctx) {
		av_freep(&avio_ctx->buffer);
		av_freep(&avio_ctx);
	}

	if (ret < 0) {
		return false;
	}

	return true;
}

