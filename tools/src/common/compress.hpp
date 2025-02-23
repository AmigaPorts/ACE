#ifndef _ACE_TOOLS_COMMON_COMPRESS_H_
#define _ACE_TOOLS_COMMON_COMPRESS_H_

#include <cstdint>
#include <cstddef>

struct tCompressUnpackState {
	std::uint8_t table[0x1000];
	const std::uint8_t *pSrc;
	std::size_t srcSize;
	std::uint8_t *pDst;
	std::size_t dstSize;
	bool isVerbose;
	unsigned int bit;
	unsigned int rlePos;
	unsigned int rleLength;
	unsigned int src_offset;
	unsigned int dst_offset;
	unsigned int table_index;
	unsigned int rleIndex;
	std::uint16_t word;
	std::uint8_t ctrl_byte;
	std::uint8_t byte;
};

size_t compressPack(
	const uint8_t *pSrc, size_t srcSize, uint8_t *pDst, size_t dstSize,
	bool isVerbose = false
);

void compressUnpackStateInit(
	tCompressUnpackState *pState, const uint8_t *pSrc, size_t srcSize,
	uint8_t *pDst, size_t dstSize, bool isVerbose = false
);

void compressUnpackProcess(tCompressUnpackState *State);

#endif // _ACE_TOOLS_COMMON_COMPRESS_H_
