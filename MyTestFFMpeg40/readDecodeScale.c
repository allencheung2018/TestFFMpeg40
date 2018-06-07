#include <libavformat/avformat.h>
#include <libavutil/file.h>
#include <libavformat/avio.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


struct buffer_data {
	uint8_t *ptr;
	size_t size; ///< size left in the buffer
};

struct SwsContext *sws_ctx;

static enum AVPixelFormat pix_fmt, dst_pix_fmt = AV_PIX_FMT_BGR24;

static FILE *video_dst_file = NULL, *dst_file = NULL;;

static uint8_t *video_dst_data[4] = { NULL }, *dst_data[4];
static int  video_dst_linesize[4], dst_linesize[4];
static int video_dst_bufsize, dst_bufsize;;

static int width, height;

static int read_packet(void *opaque, uint8_t *buf, int buf_size)
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

int refcount = 0;
static int open_codec_context(int *stream_idx,
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
		av_dict_set(&opts, "refcounted_frames", refcount ? "1" : "0", 0);
		if ((ret = avcodec_open2(*dec_ctx, dec, &opts)) < 0) {
			fprintf(stderr, "Failed to open %s codec\n",
				av_get_media_type_string(type));
			return ret;
		}
		*stream_idx = stream_index;
	}

	return 0;
}

static void decode(AVCodecContext *dec_ctx, AVFrame *frame, AVPacket *pkt,
	const char *filename)
{
	char buf[1024];
	int ret;

	ret = avcodec_send_packet(dec_ctx, pkt);
	if (ret < 0) {
		fprintf(stderr, "Error sending a packet for decoding\n");
		exit(1);
	}
	printf("avcodec_send_packet ret=%d\n", ret);
	while (ret >= 0) {
		ret = avcodec_receive_frame(dec_ctx, frame);
		printf("avcodec_receive_frame ret=%d\n", ret);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			return;
		else if (ret < 0) {
			fprintf(stderr, "Error during decoding\n");
			exit(1);
		}
		printf("output fmt:%s, width=%d, height=%d\n", av_get_pix_fmt_name(frame->format), frame->width, frame->height);

		printf("saving frame %3d\n", dec_ctx->frame_number);
		fflush(stdout);

		/* copy decoded frame to destination buffer:
		* this is required since rawvideo expects non aligned data */
		av_image_copy(video_dst_data, video_dst_linesize,
			(const uint8_t **)(frame->data), frame->linesize,
			pix_fmt, width, height);

		/* write to rawvideo file */
		fwrite(video_dst_data[0], 1, video_dst_bufsize, video_dst_file);

		frame->data[0] += frame->linesize[0] * (dec_ctx->height - 1);
		frame->linesize[0] *= -1;
		frame->data[1] += frame->linesize[1] * (dec_ctx->height / 2 - 1);
		frame->linesize[1] *= -1;
		frame->data[2] += frame->linesize[2] * (dec_ctx->height / 2 - 1);
		frame->linesize[2] *= -1;

		/* convert to destination format */
		sws_scale(sws_ctx, (const uint8_t * const*)(frame->data),
			frame->linesize, 0, frame->height, dst_data, dst_linesize);
		/* write scaled image to file */
		fwrite(dst_data[0], 1, dst_bufsize, dst_file);
	}
}

int main(int argc, char *argv[])
{
	printf("start\n");
	char videoSrcPath[] = "D:\\YNZZ_Info\\DM3730\\Live555_Debug\\Live555_Windows\\Debug\\海康加头.h265";
	char videoH264SrcPath1[] = "E:\\Video\\VideoFromCam\\1B6D5B0E-AB34-498D-BCA3-102E46DA8415.h264";
	char videoH264SrcPath2[] = "E:\\Video\\VideoFromCam\\2-0522_094015-0522_094025.h264";
	char videoH264Pic[] = "E:\\Video\\VideoFromCam\\抓图h264\\captureImg_channel4_20180517-142618.h264";
	char video1_dst_filename[] = "outVideo1.yuv";
	char video2_dst_filename[] = "outVideo2.rgb";
	FILE *inputF=NULL;
	int ret = 0;
	const int INPUTLEN = 500 * 1024;
	AVFormatContext *fmt_ctx = NULL;
	AVIOContext *avio_ctx = NULL;
	uint8_t *avio_ctx_buffer = NULL;
	size_t avio_ctx_buffer_size = 4096;
	struct buffer_data bd = { 0 };
	int video_stream_idx = -1;
	AVCodecContext *video_dec_ctx = NULL;
	AVPacket *pkt;							//存放压缩的视频数据
	int dst_w=352, dst_h=288;

//
	video_dst_file = fopen(video1_dst_filename, "wb");
	if (!video_dst_file) {
		fprintf(stderr, "Could not open destination file %s\n", video1_dst_filename);
		ret = 1;
		exit(1);
	}

	dst_file = fopen(video2_dst_filename, "wb");
	if (!dst_file) {
		fprintf(stderr, "Could not open destination file %s\n", video2_dst_filename);
		ret = 1;
		exit(1);
	}

	printf("input file:%s out file:%s\n", videoH264Pic, video1_dst_filename);
	inputF = fopen(videoH264Pic, "rb");
	if (!inputF) {
		fprintf(stderr, "Could not open %s\n", videoH264Pic);
		exit(1);
	}
	uint8_t inputBuf[500*1024];
	int len = 0, rl=1;
	uint8_t buf[1024];
	//while (len < 40*1024)
	while (rl>0 && len < INPUTLEN -rl)
	{
		rl = fread(buf, 1, 1024, inputF);
		//printf("read rl = %d bytes\n", rl);
		memcpy(inputBuf + len, buf, rl);
		len += rl;
		//printf("read len = %d bytes\n", len);
	}
	printf("read len = %d bytes\n", len);

	/* fill opaque structure used by the AVIOContext read callback */
	bd.ptr = inputBuf;
	bd.size = len;
	
	//获取buffer中的媒体信息
	if (!(fmt_ctx = avformat_alloc_context())) {
		ret = AVERROR(ENOMEM);
		goto end;
	}

	avio_ctx_buffer = (uint8_t*)av_malloc(avio_ctx_buffer_size);
	if (!avio_ctx_buffer) {
		ret = AVERROR(ENOMEM);
		goto end;
	}

	avio_ctx = avio_alloc_context(avio_ctx_buffer, avio_ctx_buffer_size,
		0, &bd, &read_packet, NULL, NULL);

	if (!avio_ctx) {
		ret = AVERROR(ENOMEM);
		goto end;
	}
	fmt_ctx->pb = avio_ctx;

	ret = avformat_open_input(&fmt_ctx, NULL, NULL, NULL);
	if (ret < 0) {
		fprintf(stderr, "Could not open input\n");
		goto end;
	}

	ret = avformat_find_stream_info(fmt_ctx, NULL);
	if (ret < 0) {
		fprintf(stderr, "Could not find stream information\n");
		goto end;
	}

	av_dump_format(fmt_ctx, 0, videoH264SrcPath1, 0);

	//printf("video type:%d\n", fmt_ctx->video_codec->id);

	if (open_codec_context(&video_stream_idx, &video_dec_ctx, fmt_ctx, AVMEDIA_TYPE_VIDEO) >= 0) {
		//video_stream = fmt_ctx->streams[video_stream_idx];

		/* allocate image where the decoded image will be put */
		width = video_dec_ctx->width;
		height = video_dec_ctx->height;
		pix_fmt = video_dec_ctx->pix_fmt;
		ret = av_image_alloc(video_dst_data, video_dst_linesize,
			width, height, pix_fmt, 1);
		//if (ret < 0) {
		//	fprintf(stderr, "Could not allocate raw video buffer\n");
		//	goto end;
		//}
		video_dst_bufsize = ret;
	}

	//缩放输出buffer初始化
	/* buffer is going to be written to rawvideo file, no alignment */
	if ((ret = av_image_alloc(dst_data, dst_linesize,
								dst_w, dst_h, dst_pix_fmt, 1)) < 0) {
		fprintf(stderr, "Could not allocate destination image\n");
		goto end;
	}
	dst_bufsize = ret;

	printf("video width=%d height=%d fmt:%s\n", width, height, av_get_pix_fmt_name(pix_fmt));
	printf("id:%d\n", video_dec_ctx->codec_id);

	/*
	解码部分
	*/
	pkt = av_packet_alloc();
	if (!pkt)
		exit(1);

	AVCodecParserContext *parser;
	parser = av_parser_init(video_dec_ctx->codec_id);
	if (!parser) {
		fprintf(stderr, "parser not found\n");
		exit(1);
	}

	AVFrame *frame;
	frame = av_frame_alloc();
	if (!frame) {
		fprintf(stderr, "Could not allocate video frame\n");
		exit(1);
	}
	/*
	输出格式缩放
	*/
	/* create scaling context */
	sws_ctx = sws_getContext(width, height, pix_fmt,
		dst_w, dst_h, dst_pix_fmt,
		SWS_BILINEAR, NULL, NULL, NULL);

	/* use the parser to split the data into frames */
	printf("input buf len=%d\n", len);
	uint8_t *data = inputBuf;
	while (len > 0) {
		ret = av_parser_parse2(parser, video_dec_ctx, &pkt->data, &pkt->size,
			data, len, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
		if (ret < 0) {
			fprintf(stderr, "Error while parsing\n");
			exit(1);
		}
	
		data += ret;
		len -= ret;
		printf("parser ret=%d pkt size=%d len=%d\n", ret, pkt->size,len);
		if (pkt->size)
			decode(video_dec_ctx, frame, pkt, videoH264Pic);
	}

	/* flush the decoder */
	decode(video_dec_ctx, frame, NULL, videoH264Pic);


//
end:
	avformat_close_input(&fmt_ctx);
	/* note: the internal buffer could have changed, and be != avio_ctx_buffer */
	if (avio_ctx) {
		av_freep(&avio_ctx->buffer);
		av_freep(&avio_ctx);
	}
	avcodec_free_context(&video_dec_ctx);
	av_parser_close(parser);
	av_frame_free(&frame);
	av_packet_free(&pkt);

	if (ret < 0) {
		fprintf(stderr, "Error occurred: %s\n", av_err2str(ret));
		return 1;
	}

	fclose(inputF);
	fclose(dst_file);
	fclose(video_dst_file);

	printf("run to end.");
	return 0;
}