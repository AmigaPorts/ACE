#ifndef _ACE_TOOLS_COMMON_COMPRESS_H_
#define _ACE_TOOLS_COMMON_COMPRESS_H_

#include <cstdint>

size_t compressPack(
	const uint8_t *pSrc, size_t srcSize, uint8_t *pDst, size_t dstSize,
	bool isVerbose = false
);

void compressUnpack(
	const uint8_t *pSrc, size_t srcSize, uint8_t *pDst, size_t dstSize,
	bool isVerbose = false
);

#endif // _ACE_TOOLS_COMMON_COMPRESS_H_
