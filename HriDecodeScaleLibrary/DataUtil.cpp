#include "DataUtil.h"


int findStartPostion(uint8_t *src, int lenSrc, int offset, const uint8_t *needFind, int lenNF) {
	for (int i = offset; i<lenSrc - lenNF; i++) {
		bool isValid = true;
		for (int j = 0; j<lenNF; j++) {
			if (src[i + j] != needFind[j]) {
				isValid = false;
				break;
			}
		}
		if (isValid) {
			return i;
		}
	}
	return -1;
}