#include <libavformat/avformat.h>
#include <libavutil/file.h>
#include <libavformat/avio.h>
#include <libavcodec/avcodec.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct buffer_data {
	uint8_t *ptr;
	size_t size; ///< size left in the buffer
};

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

int main(int argc, char *argv[])
{
	printf("start");
//	char videoSrcPath[100] = "D:\\YNZZ_Info\\DM3730\\Live555_Debug\\Live555_Windows\\Debug\\海康加头.h265";
//	FILE *inputF;
//	int ret = 0;
//	uint8_t inputBuf[1024 * 1024];
//	AVFormatContext *fmt_ctx = NULL;
//	AVIOContext *avio_ctx = NULL;
//	uint8_t *avio_ctx_buffer = NULL;
//	size_t avio_ctx_buffer_size = 4096;
//	struct buffer_data bd = { 0 };
//
//	printf("input file:s%", videoSrcPath);
//	inputF = fopen(videoSrcPath, "rb");
//	if (!inputF) {
//		fprintf(stderr, "Could not open %s\n", videoSrcPath);
//		exit(1);
//	}
//	int len = 0;
//	if (len<10 * 1024)
//	{
//		int rl = fread(inputBuf, 1, 1024, inputF);
//		len += rl;
//		printf("read %d bytes", len);
//	}
//
//	/* fill opaque structure used by the AVIOContext read callback */
//	bd.ptr = inputBuf;
//	bd.size = len;
//
//	//获取buffer中的媒体信息
//	if (!(fmt_ctx = avformat_alloc_context())) {
//		ret = AVERROR(ENOMEM);
//		goto end;
//	}
//
//	avio_ctx_buffer = (uint8_t*)av_malloc(avio_ctx_buffer_size);
//	if (!avio_ctx_buffer) {
//		ret = AVERROR(ENOMEM);
//		goto end;
//	}
//
//	avio_ctx = avio_alloc_context(avio_ctx_buffer, avio_ctx_buffer_size,
//		0, &bd, &read_packet, NULL, NULL);
//
//	if (!avio_ctx) {
//		ret = AVERROR(ENOMEM);
//		goto end;
//	}
//	fmt_ctx->pb = avio_ctx;
//
//	ret = avformat_open_input(&fmt_ctx, NULL, NULL, NULL);
//	if (ret < 0) {
//		fprintf(stderr, "Could not open input\n");
//		goto end;
//	}
//
//	ret = avformat_find_stream_info(fmt_ctx, NULL);
//	if (ret < 0) {
//		fprintf(stderr, "Could not find stream information\n");
//		goto end;
//	}
//
//	av_dump_format(fmt_ctx, 0, videoSrcPath, 0);
//
//end:
//	avformat_close_input(&fmt_ctx);
//	/* note: the internal buffer could have changed, and be != avio_ctx_buffer */
//	if (avio_ctx) {
//		av_freep(&avio_ctx->buffer);
//		av_freep(&avio_ctx);
//	}
//
//	if (ret < 0) {
//		fprintf(stderr, "Error occurred: %s\n", av_err2str(ret));
//		return 1;
//	}
//
//	fclose(inputF);
}