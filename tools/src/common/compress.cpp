#include "../common/compress.hpp"
#include <cmath>
#include <fmt/format.h>

static constexpr auto s_RleMinLength = 3u;
static constexpr auto s_RleMaxLength = 18u;

static uint8_t rleTableRead(const uint8_t *table, unsigned *index)
{
	uint8_t byte;

	byte = table[*index];
	(*index)++;
	(*index) &= 0xfff;

	return byte;
}

static bool rleTableFind(
	const uint8_t *table, std::uint16_t table_index,
	const uint8_t *data, size_t data_size,
	std::uint16_t *index, std::uint8_t *length
)
{
	*index = 0;
	*length = 0;
	unsigned int seq_len;
	bool isFound = false;
	for (auto i = 0u; i < 0x1000; i++) {
		for (seq_len = 0u; seq_len < data_size; seq_len++) {
			auto offset = i + seq_len;
			if (table[offset & 0xfff] != data[seq_len]) {
				break;
			}

			/*
			 * Don't return RLE runs that are in the area of the table that
			 * will be written to, since the values will change.
			 */
			if (
				(i >= table_index && i <= table_index + seq_len) ||
				(offset >= table_index && offset <= table_index + seq_len)
			) {
				seq_len = 0;
				break;
			}
		}
		if (seq_len >= s_RleMinLength) {
			if(seq_len > *length) {
				*index = i;
				*length = seq_len;
				isFound = true;
			}
		}
	}

	return isFound;
}

static void rleTableWrite(uint8_t *table, unsigned *index, uint8_t byte)
{
	table[*index] = byte;
	(*index)++;
	(*index) &= 0xfff;
}

void compressUnpackStateInit(
	tCompressUnpackState *pState, const uint8_t *pCompressed, size_t ulCompressedSize,
	size_t ulUncompressedSize, bool isVerbose
)
{
	memset(pState, 0, sizeof(*pState));
	pState->pCompressed = pCompressed;
	pState->ulCompressedSize = ulCompressedSize;
	pState->ulUncompressedSize = ulUncompressedSize;
	pState->isVerbose = isVerbose;
}

tCompressUnpackResult compressUnpackProcess(
	tCompressUnpackState *pState, std::uint8_t *pOut
)
{
	if(pState->isVerbose) fmt::println("Decompress start");

	switch(pState->eCurrentState) {
		case COMPRESS_UNPACK_STATE_KIND_READ_CTL:
			pState->ctl_offset = pState->src_offset;
			pState->ctrl_byte = pState->pCompressed[pState->src_offset++];
			pState->bit = 0;
			pState->eCurrentState = COMPRESS_UNPACK_STATE_KIND_PROCESS_CTL_BIT;
			break;

		case COMPRESS_UNPACK_STATE_KIND_PROCESS_CTL_BIT:
			if(pState->bit < 8 && pState->dst_offset < pState->ulUncompressedSize) {
				std::uint8_t ubBitValue = pState->ctrl_byte & (1 << pState->bit);
				pState->bit++;
				if (ubBitValue) {
					/* Output the next byte and store it in the table */
					pState->byte = pState->pCompressed[pState->src_offset++];
					if(pState->isVerbose) fmt::println("byte at {}: {:02X}", pState->src_offset - 1, pState->byte);
					rleTableWrite(pState->table, &pState->table_index, pState->byte);
					*pOut = pState->byte;
					pState->dst_offset++;
					return COMPRESS_UNPACK_RESULT_BUSY_WROTE_BYTE;
				}
				else {
					pState->word = (pState->pCompressed[pState->src_offset + 0] << 0) | (pState->pCompressed[pState->src_offset + 1] << 8);
					pState->src_offset += 2;

					pState->rleLength = ((pState->word & 0xf000) >> 12) + 3;
					pState->rleIndex = pState->word & 0xfff;

					if(pState->isVerbose) fmt::print(
						"sequence at {}, word: {:04X}, len: {}, index: {}, sequence:",
						pState->src_offset - 2, pState->word, pState->rleLength, pState->rleIndex
					);
					pState->rlePos = 0;
					pState->eCurrentState = COMPRESS_UNPACK_STATE_KIND_WRITE_RLE;
				}
			}
			else {
				pState->eCurrentState = COMPRESS_UNPACK_STATE_KIND_END_CTL;
			}
			break;

		case COMPRESS_UNPACK_STATE_KIND_END_CTL:
			if(pState->isVerbose) fmt::println("used ctl at {}: {:02X}", pState->ctl_offset, pState->ctrl_byte);
			if (pState->dst_offset >= pState->ulUncompressedSize) {
				pState->eCurrentState = COMPRESS_UNPACK_STATE_KIND_DONE;
			}
			else {
				pState->eCurrentState = COMPRESS_UNPACK_STATE_KIND_READ_CTL;
			}
			break;

		case COMPRESS_UNPACK_STATE_KIND_WRITE_RLE:
			if(pState->rlePos < pState->rleLength) {
				pState->byte = rleTableRead(pState->table, &pState->rleIndex);
				if(pState->isVerbose) fmt::print(" {:02X}", pState->byte);
				rleTableWrite(pState->table, &pState->table_index, pState->byte);
				*pOut = pState->byte;
				pState->dst_offset++;
			++pState->rlePos;
				return COMPRESS_UNPACK_RESULT_BUSY_WROTE_BYTE;
			}
			else {
				if(pState->isVerbose) fmt::print("\n");
				pState->eCurrentState = COMPRESS_UNPACK_STATE_KIND_PROCESS_CTL_BIT;
			}
			break;

		case COMPRESS_UNPACK_STATE_KIND_DONE:
			if(pState->isVerbose) fmt::println("Decompress done");
			return COMPRESS_UNPACK_RESULT_DONE;
			break;
	}

	return COMPRESS_UNPACK_RESULT_BUSY;
}

size_t compressPack(
	const uint8_t *src, size_t src_size,
	uint8_t *dst, size_t dst_size, bool isVerbose
) {
	if(isVerbose) fmt::println("Compress start, size {}", src_size);
	uint8_t table[0x1000] = {0}, byte;
	unsigned i, bit, src_offset = 0, dst_offset = 0, table_index = 0, ctrl_byte_offset;
	std::uint16_t rle_index;
	std::uint8_t rle_len;
	uint16_t word;
	bool found;

	while (src_offset < src_size) {
		/* Write a place-holder control byte */
		ctrl_byte_offset = dst_offset++;
		dst[ctrl_byte_offset] = 0;

		for (bit = 0; bit < 8; bit++) {
			if (src_offset >= src_size) {
				break;
			}

			/* Try to find an repeated sequence */
			found = rleTableFind(
				table, table_index, &src[src_offset],
				std::min<std::size_t>(src_size - src_offset, s_RleMaxLength),
				&rle_index, &rle_len
			);
			if (found) {
				/*
				 * RLE sequence found. Encode a 16-bit word for length
				 * and index. Control byte flag is not set.
				 */
				word = ((rle_len - 3) & 0xf) << 12;
				word |= rle_index & 0xfff;

				if(isVerbose) fmt::println(
					"sequence at {}, word: {:04X}, len: {}, index: {}, sequence: {:02X}",
					dst_offset, word, rle_len, rle_index,
					fmt::join(&src[src_offset], &src[src_offset + rle_len], " ")
				);
				dst[dst_offset++] = std::uint8_t(word);
				dst[dst_offset++] = word >> 8;

				for (i = 0; i < rle_len; i++) {
					rleTableWrite(table, &table_index, src[src_offset++]);
				}
			}
			else {
				/*
				 * No sequence found. Encode the byte directly.
				 * Control byte flag is set.
				 */
				dst[ctrl_byte_offset] |= (1 << bit);
				byte = src[src_offset++];
				if(isVerbose) fmt::println("byte at {}: {:02X}", dst_offset, byte);
				dst[dst_offset++] = byte;
				rleTableWrite(table, &table_index, byte);
			}
		}
		if(isVerbose) fmt::println("used ctl at {}: {:02X}", ctrl_byte_offset, dst[ctrl_byte_offset]);
	}

	if(isVerbose) fmt::println("compress done, length: {}", dst_offset);
	return dst_offset;
}
