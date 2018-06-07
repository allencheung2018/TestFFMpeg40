#include <stdio.h>
#include <map>
#include <mutex>

#include "MediaConvertor.h"
#include "UseMediaConvertor.h"

using namespace std;

static int indexCls = 0;
static map<int, MediaConvertor*> mcvtMap;
static mutex g_lock;

int createMediaConvertor(int outFmt, int outWidth, int outHeight) {
	printf("createMediaConvertor outFmt:%d width=%d height=%d\n", outFmt, outWidth, outHeight);
	g_lock.lock();
	indexCls += 1;
	if (indexCls == 32)
	{
		indexCls = 0;
	}
	MediaConvertor *mcvt = new MediaConvertor(outFmt, outWidth, outHeight);
	mcvtMap[indexCls] = mcvt;

	printf("createMediaConvertor id=%d\n", indexCls);
	g_lock.unlock();
	return indexCls;
}

int decodeScaleMedia(int id, int playType, uint8_t *src, int len, uint8_t *out, int &oWidth, float &fps) {
	MediaConvertor *mcvt = mcvtMap.at(id);
	return mcvt->manageFrame(playType, src, len, out, oWidth, fps);
}

int deleteMediaConvertor(int id) {
	MediaConvertor *mcvt = mcvtMap.at(id);
	delete mcvt;
	int re = mcvtMap.erase(id);
	printf("deleteMediaConvertor id=%d size=%u re=%d\n", id, mcvtMap.size(), re);
	return id;
}

int decodeScaleImage()
{
	return 0;
}
