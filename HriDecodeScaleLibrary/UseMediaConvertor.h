#pragma once

extern int createMediaConvertor(int outFmt, int outWidth, int outHeight);
extern int decodeScaleMedia(int id, int playType, uint8_t *src, int len, uint8_t *out, int &oW, int &oH, float &fps);
extern int deleteMediaConvertor(int id);
