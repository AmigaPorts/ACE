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

void compressUnpack(
	const uint8_t *pSrc, size_t src_size,
	uint8_t *pDst, size_t dst_size, bool isVerbose
)
{
	unsigned bit, i, count, src_offset = 0, dst_offset = 0, table_index = 0, rle_index;
	uint8_t table[0x1000] = {0};
	uint16_t word;
	uint8_t ctrl_byte, byte;

	if(isVerbose) fmt::println("Decompress start");
	while (dst_offset < dst_size) {
		auto ctl_offset = src_offset;
		ctrl_byte = pSrc[src_offset++];
		for (bit = 0; bit < 8; bit++) {
			if (dst_offset >= dst_size) {
				break;
			}

			if (ctrl_byte & (1 << bit)) {
				/* Output the next byte and store it in the table */
				byte = pSrc[src_offset++];
				if(isVerbose) fmt::println("byte at {}: {:02X}", src_offset - 1, byte);
				pDst[dst_offset++] = byte;
				rleTableWrite(table, &table_index, byte);
			}
			else {
				/*
				 *
				 */
				word = (pSrc[src_offset + 0] << 0) | (pSrc[src_offset + 1] << 8);
				src_offset += 2;

				count = ((word & 0xf000) >> 12) + 3;
				rle_index = word & 0xfff;

				if(isVerbose) fmt::print(
					"sequence at {}, word: {:04X}, len: {}, index: {}, sequence:",
					src_offset - 2, word, count, rle_index
				);

				for (i = 0; i < count; i++) {
					byte = rleTableRead(table, &rle_index);
					if(isVerbose) fmt::print(" {:02X}", byte);
					rleTableWrite(table, &table_index, byte);
					pDst[dst_offset++] = byte;
				}
				if(isVerbose) fmt::print("\n");
			}
		}
		if(isVerbose) fmt::println("used ctl at {}: {:02X}", ctl_offset, ctrl_byte);
	}
	if(isVerbose) fmt::println("Decompress done");
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
