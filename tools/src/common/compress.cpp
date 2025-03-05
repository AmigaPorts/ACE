#include "../common/compress.hpp"
#include <cmath>
#include <fmt/format.h>

static constexpr auto s_RleMinLength = 3u;
static constexpr auto s_RleMaxLength = 18u;

static uint8_t rleTableRead(const uint8_t *table, std::uint16_t *index)
{
	uint8_t byte;

	byte = table[*index];
	(*index)++;
	(*index) &= 0xfff;

	return byte;
}

static bool rleTableFind(
	const uint8_t *pLookup, std::uint16_t uwLookupLength,
	std::uint16_t uwLookupWritePos, const uint8_t *pData, std::uint32_t ulMatchLimit,
	std::uint16_t *pMatchPosition, std::uint8_t *pMatchLength
)
{
	*pMatchPosition = 0;
	*pMatchLength = 0;
	std::uint32_t ulMatchedLength;
	bool isFound = false;
	for (auto i = 0u; i < uwLookupLength; i++) {
		std::uint32_t ulLimit = ulMatchLimit;
		if(uwLookupLength < 0x1000) {
			ulLimit = std::min(ulLimit, uwLookupLength - i);
		}
		for (ulMatchedLength = 0u; ulMatchedLength < ulLimit; ulMatchedLength++) {
			auto offset = i + ulMatchedLength;
			if (pLookup[offset & 0xfff] != pData[ulMatchedLength]) {
				break;
			}

			 // Don't return RLE runs that are in the area of the table that
			 // will be written to, since the values will change.
			if (
				(i >= uwLookupWritePos && i <= uwLookupWritePos + ulMatchedLength) ||
				(offset >= uwLookupWritePos && offset <= uwLookupWritePos + ulMatchedLength)
			) {
				ulMatchedLength = 0;
				break;
			}
		}
		if (ulMatchedLength >= s_RleMinLength) {
			if(ulMatchedLength > *pMatchLength) {
				*pMatchPosition = i;
				*pMatchLength = ulMatchedLength;
				isFound = true;
			}
		}
	}

	return isFound;
}

static void rleTableWrite(uint8_t *table, std::uint16_t *index, uint8_t byte)
{
	table[*index] = byte;
	(*index)++;
	(*index) &= 0xfff;
}

void compressUnpackerInit(
	tCompressUnpacker *pUnpacker, const uint8_t *pCompressed, size_t ulCompressedSize,
	size_t ulUncompressedSize, bool isVerbose
)
{
	memset(pUnpacker, 0, sizeof(*pUnpacker));
	for(std::uint16_t i = 0; i < 0x1000; ++i) {
		pUnpacker->pLookup[i] = rand();
	}
	pUnpacker->pCompressed = pCompressed;
	pUnpacker->ulCompressedSize = ulCompressedSize;
	pUnpacker->ulUncompressedSize = ulUncompressedSize;
	pUnpacker->isVerbose = isVerbose;
	if(pUnpacker->isVerbose) fmt::println("Decompress start");
}

tCompressUnpackResult compressUnpackerProcess(
	tCompressUnpacker *pUnpacker, std::uint8_t *pOut
)
{
	switch(pUnpacker->eCurrentState) {
		case COMPRESS_UNPACK_STATE_KIND_READ_CTL:
			pUnpacker->ulCtlOffset = pUnpacker->ulReadOffset;
			pUnpacker->ubCtlByte = pUnpacker->pCompressed[pUnpacker->ulReadOffset++];
			pUnpacker->ubCtlBitIndex = 0;
			pUnpacker->eCurrentState = COMPRESS_UNPACK_STATE_KIND_PROCESS_CTL_BIT;
			break;

		case COMPRESS_UNPACK_STATE_KIND_PROCESS_CTL_BIT:
			if(pUnpacker->ubCtlBitIndex < 8 && pUnpacker->ulWriteOffset < pUnpacker->ulUncompressedSize) {
				std::uint8_t ubBitValue = pUnpacker->ubCtlByte & (1 << pUnpacker->ubCtlBitIndex);
				pUnpacker->ubCtlBitIndex++;
				if (ubBitValue) {
					// Output the next byte and store it in the table
					std::uint8_t ubRawByte = pUnpacker->pCompressed[pUnpacker->ulReadOffset++];
					if(pUnpacker->isVerbose) fmt::println("byte at {}: {:02X}", pUnpacker->ulReadOffset - 1, ubRawByte);
					rleTableWrite(pUnpacker->pLookup, &pUnpacker->uwLookupPos, ubRawByte);
					*pOut = ubRawByte;
					pUnpacker->ulWriteOffset++;
					return COMPRESS_UNPACK_RESULT_BUSY_WROTE_BYTE;
				}
				else {
					std::uint16_t uwRleCtl = (
						(pUnpacker->pCompressed[pUnpacker->ulReadOffset + 0] << 8) |
						(pUnpacker->pCompressed[pUnpacker->ulReadOffset + 1] << 0)
					);
					pUnpacker->ulReadOffset += 2;

					pUnpacker->ubRleLength = (uwRleCtl & 0xf) + 3;
					pUnpacker->uwRleStart = uwRleCtl >> 4;

					if(pUnpacker->isVerbose) fmt::print(
						"sequence at {}, word: {:04X}, len: {}, index: {}, sequence:",
						pUnpacker->ulReadOffset - 2, uwRleCtl, pUnpacker->ubRleLength, pUnpacker->uwRleStart
					);
					pUnpacker->ubRlePos = 0;
					pUnpacker->eCurrentState = COMPRESS_UNPACK_STATE_KIND_WRITE_RLE;
				}
			}
			else {
				pUnpacker->eCurrentState = COMPRESS_UNPACK_STATE_KIND_END_CTL;
			}
			break;

		case COMPRESS_UNPACK_STATE_KIND_END_CTL:
			if(pUnpacker->isVerbose) fmt::println("used ctl at {}: {:02X}", pUnpacker->ulCtlOffset, pUnpacker->ubCtlByte);
			if (pUnpacker->ulWriteOffset >= pUnpacker->ulUncompressedSize) {
				pUnpacker->eCurrentState = COMPRESS_UNPACK_STATE_KIND_DONE;
			}
			else {
				pUnpacker->eCurrentState = COMPRESS_UNPACK_STATE_KIND_READ_CTL;
			}
			break;

		case COMPRESS_UNPACK_STATE_KIND_WRITE_RLE:
			if(pUnpacker->ubRlePos < pUnpacker->ubRleLength) {
				std::uint8_t ubRawByte = rleTableRead(pUnpacker->pLookup, &pUnpacker->uwRleStart);
				if(pUnpacker->isVerbose) fmt::print(" {:02X}", ubRawByte);
				rleTableWrite(pUnpacker->pLookup, &pUnpacker->uwLookupPos, ubRawByte);
				*pOut = ubRawByte;
				pUnpacker->ulWriteOffset++;
				++pUnpacker->ubRlePos;
				return COMPRESS_UNPACK_RESULT_BUSY_WROTE_BYTE;
			}
			else {
				if(pUnpacker->isVerbose) fmt::print("\n");
				pUnpacker->eCurrentState = COMPRESS_UNPACK_STATE_KIND_PROCESS_CTL_BIT;
			}
			break;

		case COMPRESS_UNPACK_STATE_KIND_DONE:
			if(pUnpacker->isVerbose) fmt::println("Decompress done");
			return COMPRESS_UNPACK_RESULT_DONE;
			break;
	}

	return COMPRESS_UNPACK_RESULT_BUSY;
}

std::uint32_t compressPack(
	const uint8_t *pSrc, std::uint32_t ulSrcSize,
	uint8_t *pDest, bool isVerbose
) {
	if(isVerbose) fmt::println("Compress start, size {}", ulSrcSize);
	std::uint8_t pLookup[0x1000] = {0};
	std::uint32_t ulSrcOffset = 0, ulDestOffset = 0, ulCtrlByteOffset;
	std::uint16_t uwLookupWritePos = 0;
	std::uint16_t uwRleMatchPosition;
	std::uint8_t ubRleMatchLength;
	std::uint16_t uwLookupLength = 0;
	std::uint16_t uwRleCtl;

	while (ulSrcOffset < ulSrcSize) {
		// Write a place-holder control byte
		ulCtrlByteOffset = ulDestOffset++;
		pDest[ulCtrlByteOffset] = 0;

		for (std::uint8_t ubBit = 0; ubBit < 8; ubBit++) {
			if (ulSrcOffset >= ulSrcSize) {
				break;
			}

			// Try to find an repeated sequence
			bool isFound = rleTableFind(
				pLookup, uwLookupLength, uwLookupWritePos, &pSrc[ulSrcOffset],
				std::min(ulSrcSize - ulSrcOffset, s_RleMaxLength),
				&uwRleMatchPosition, &ubRleMatchLength
			);
			if (isFound) {
				// RLE sequence found. Encode a 16-bit word for length
				// and index. Control byte flag is not set.
				uwRleCtl = (ubRleMatchLength - 3) & 0xf;
				uwRleCtl |= (uwRleMatchPosition & 0xfff) << 4;

				if(isVerbose) fmt::println(
					"sequence at {}, word: {:04X}, len: {}, index: {}, sequence: {:02X}",
					ulDestOffset, uwRleCtl, ubRleMatchLength, uwRleMatchPosition,
					fmt::join(&pSrc[ulSrcOffset], &pSrc[ulSrcOffset + ubRleMatchLength], " ")
				);
				pDest[ulDestOffset++] = uwRleCtl >> 8;
				pDest[ulDestOffset++] = std::uint8_t(uwRleCtl);

				for (std::uint8_t i = 0; i < ubRleMatchLength; i++) {
					rleTableWrite(pLookup, &uwLookupWritePos, pSrc[ulSrcOffset++]);
					uwLookupLength = std::min(0x1000, uwLookupLength + 1);
				}
			}
			else {
				// No sequence found. Encode the byte directly.
				// Control byte flag is set.
				pDest[ulCtrlByteOffset] |= (1 << ubBit);
				auto RawByte = pSrc[ulSrcOffset++];
				if(isVerbose) fmt::println("byte at {}: {:02X}", ulDestOffset, RawByte);
				pDest[ulDestOffset++] = RawByte;
				rleTableWrite(pLookup, &uwLookupWritePos, RawByte);
				uwLookupLength = std::min(0x1000, uwLookupLength + 1);
			}
		}
		if(isVerbose) fmt::println("used ctl at {}: {:02X}", ulCtrlByteOffset, pDest[ulCtrlByteOffset]);
	}

	if(isVerbose) fmt::println("compress done, length: {}", ulDestOffset);
	return ulDestOffset;
}
