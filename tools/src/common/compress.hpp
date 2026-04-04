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

struct tCompressUnpacker {
	tCompressUnpackStateKind eCurrentState;
	std::uint8_t pLookup[0x1000];
	const std::uint8_t *pCompressed;
	std::size_t ulCompressedSize;
	std::size_t ulUncompressedSize;
	bool isVerbose;
	std::uint8_t ubCtlByte;
	std::uint8_t ubCtlBitIndex;
	std::uint8_t ubRlePos;
	std::uint8_t ubRleLength;
	std::uint16_t uwRleStart;
	std::uint16_t uwLookupPos;
	std::uint32_t ulReadOffset;
	std::uint32_t ulWriteOffset;
	std::uint32_t ulCtlOffset;
};

std::uint32_t compressPack(
	const uint8_t *pSrc, std::uint32_t ulSrcSize,
	uint8_t *pDest, bool isVerbose = false
);

void compressUnpackerInit(
	tCompressUnpacker *pUnpacker, const uint8_t *pCompressed, size_t ulCompressedSize,
	size_t ulUncompressedSize, bool isVerbose = false
);

tCompressUnpackResult compressUnpackerProcess(
	tCompressUnpacker *State, std::uint8_t *pOut
);

#endif // _ACE_TOOLS_COMMON_COMPRESS_H_
