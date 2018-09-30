// HriDecodeScaleLibrary.cpp : 定义控制台应用程序的入口点。
//

#include <stdio.h>
#include <stdint.h>
#include <string>
#include <thread>
#include<windows.h>

#include "DataUtil.h"
#include "UseMediaConvertor.h"

using namespace std;

static const int FILELEN = 300 * 1024;
static const uint8_t startCode[4] = { 0,0,0,0x01 };

void threadFun1(char *fn) {
	printf("threadFun1 : %s thread id=%d\n", fn, this_thread::get_id());
	FILE *File1 = fopen(fn, "rb");
	FILE *outF1 = fopen("out1.rgb", "wb");

	uint8_t buf[4096];
	uint8_t *outData1 = new uint8_t[3 * 1280 * 720];
	int oW, oH;
	float fps;

	int id1 = createMediaConvertor(2, 0, 0);
	int oLen = 2;
	while (!feof(File1))
	{
		int rl = fread(buf, 1, 4096, File1);
		int ret = decodeScaleMedia(id1, 2, buf, rl, outData1, oW, oH, fps);
		printf("ret =%d image width=%d height =%d fps=%f\n", ret, oW, oH, fps);
		if(ret>0)
			fwrite(outData1, 1, oLen*oW*oH, outF1);
		this_thread::sleep_for(chrono::milliseconds(10));
	}
	while (decodeScaleMedia(id1, 2, NULL, 0, outData1, oW, oH, fps) > 0)
	{
		fwrite(outData1, 1, oLen*oW*oH, outF1);
		Sleep(1 * 30);
	}
	deleteMediaConvertor(id1);

	delete outData1;
	fclose(File1);
	fclose(outF1);
}

void threadFun2(char *fn) {
	printf("threadFun2 : %s thread id=%d\n", fn, this_thread::get_id());
	FILE *File2 = fopen(fn, "rb");
	FILE *outF2 = fopen("out2.rgb", "wb");

	uint8_t buf[1024];
	uint8_t *inF1 = new uint8_t[20*1024*1024];
	uint8_t *outData = new uint8_t[3 * 1280 * 720];
	int oW, oH;
	float fps;

	int lenF1 = 0, rl = 1;
	while (!feof(File2))
	{
		rl = fread(buf, 1, 1024, File2);
		memcpy(inF1 + lenF1, buf, rl);
		lenF1 += rl;
	}
	printf("File2 read len = %d bytes\n", lenF1);

	int id2 = createMediaConvertor(1, 320, 0);
	int oLen = 3;
	int offset = 4, re = 0, len = 0, cnt = 0;
	while (re < lenF1)
	{
		re = findStartPostion(inF1, lenF1, offset, startCode, 4);
		printf("findStartPostion re=%d, offset=%d\n", re, offset);
		if (re == -1) {
			len = lenF1 - offset + 4;
			re = lenF1;
		}
		else {
			len = re - offset + 4;
		}
		cnt += 1;
		uint8_t *frame = new uint8_t[len];
		memcpy(frame, inF1 + re - len, len);
		printf("frame %d len=%d offset=%d re=%d lenF1=%d\n", cnt, len, offset, re, lenF1);
		int ret = decodeScaleMedia(id2, 2, frame, len, outData, oW, oH, fps);
		printf("ret image = %d image width=%d height=%d fps=%f\n", ret, oW, oH, fps);
		if (ret > 0)
		{
			fwrite(outData, 1, oLen*oW*oH, outF2);
		}
		offset = re + 4;
		delete frame;
		Sleep(10);
	}
	do
	{
		int ret = decodeScaleMedia(id2, 2, NULL, 0, outData, oW, oH, fps);
		printf("flush ret image =%d image width=%d  Height-%d fps=%f\n", ret, oW, oH, fps);
		if (ret > 0)
		{
			fwrite(outData, 1, oLen*oW*oH, outF2);
			Sleep(1 * 30);
		}
		else
		{
			break;
		}
	} while (true);

	deleteMediaConvertor(id2);
	delete outData;
	delete inF1;
	fclose(File2);
	fclose(outF2);
}


void printChar(char *fn) {
	printf("printChar fn = %s\n", fn);
	FILE *File1 = fopen(fn, "rb");
}

int main()
{
	printf("start\n");
	char video1_H265[] = "D:\\YNZZ_Info\\DM3730\\Live555_Debug\\Live555_Windows\\Debug\\大华加头.h265";
	char video2_H265[] = "D:\\YNZZ_Info\\DM3730\\Live555_Debug\\Live555_Windows\\Debug\\海康加头.h265";
	char video1_H264[] = "E:\\Video\\VideoFromCam\\1B6D5B0E-AB34-498D-BCA3-102E46DA8415.h264";
	char video2_H264[] = "E:\\Video\\VideoFromCam\\2-0522_094015-0522_094025.h264";
	char video3_H264[] = "E:\\Video\\VideoFromCam\\抓图h264\\captureImg_channel4_20180517-142618.h264";
	char video4_H264[] = "E:\\Video\\VideoFromCam\\real_847C9B43F66542E5A9977465AC767FB8_C7_20180524T092252.h264";
	char video5_H264[] = "E:\\Video\\VideoFromCam\\real_c1_20180521-172421.h264";

	thread t1(threadFun1, video1_H265);
	//thread t2(threadFun2, video1_H264);
	//thread t3(threadFun3, video1_H264);
	//thread t4(threadFun4, video2_H264);
	////image h264
	//thread t5(threadFun5, video3_H264);
	//thread t6(threadFun6, video2_H265);
	//thread t7(threadFun7, video2_H265);
	//thread t8(threadFun8, video1_H264);
	//thread t9(threadFun9, video1_H264);
	t1.join();
	//t2.join();
	//t3.join();
	//t4.join();
	//t5.join();
	//t6.join();
	//t7.join();
	//t8.join();
	//t9.join();

	//thread t3(printChar, video1_H265);
	//t3.join();

	return 0;
}

