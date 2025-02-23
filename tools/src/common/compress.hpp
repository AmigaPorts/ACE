#ifndef _ACE_TOOLS_COMMON_COMPRESS_H_
#define _ACE_TOOLS_COMMON_COMPRESS_H_

#include <cstdint>
#include <cstddef>

enum tCompressUnpackStateKind {
	COMPRESS_UNPACK_STATE_KIND_READ_CTL,
	COMPRESS_UNPACK_STATE_KIND_PROCESS_CTL_BIT,
	COMPRESS_UNPACK_STATE_KIND_END_CTL,
	COMPRESS_UNPACK_STATE_KIND_WRITE_RLE,
	COMPRESS_UNPACK_STATE_KIND_DONE,
};

enum tCompressUnpackResult {
	COMPRESS_UNPACK_RESULT_BUSY,
	COMPRESS_UNPACK_RESULT_BUSY_WROTE_BYTE,
	COMPRESS_UNPACK_RESULT_DONE,
};

struct tCompressUnpackState {
	tCompressUnpackStateKind eCurrentState;
	std::uint8_t table[0x1000];
	const std::uint8_t *pCompressed;
	std::size_t ulCompressedSize;
	std::size_t ulUncompressedSize;
	bool isVerbose;
	unsigned int bit;
	unsigned int rlePos;
	unsigned int rleLength;
	unsigned int src_offset;
	unsigned int dst_offset;
	unsigned int table_index;
	unsigned int rleIndex;
	unsigned int ctl_offset;
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
	size_t dstSize, bool isVerbose = false
);

tCompressUnpackResult compressUnpackProcess(
	tCompressUnpackState *State, std::uint8_t *pOut
);

#endif // _ACE_TOOLS_COMMON_COMPRESS_H_
