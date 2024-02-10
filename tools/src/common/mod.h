/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_TOOLS_COMMON_MOD_H_
#define _ACE_TOOLS_COMMON_MOD_H_

#include <vector>
#include <array>
#include <string>
#include <cstdint>

struct tSample {
	std::string m_szName;
	std::uint8_t m_ubFineTune; ///< Finetune. Only lower nibble is used. Values translate to finetune: {0..7, -8..-1}
	std::uint8_t m_ubVolume; ///< Sample volume. 0..64
	std::uint16_t m_uwRepeatOffs; ///< In words.
	std::uint16_t m_uwRepeatLength; ///< In words.

	std::vector<uint16_t> m_vData;

	bool operator ==(const tSample &Other) const;
	bool operator !=(const tSample &Other) const
	{
		return !(*this == Other);
	}
};

struct tNote {
	std::uint8_t ubInstrument;
	std::uint16_t uwPeriod;
	std::uint8_t ubCmd;
	std::uint8_t ubCmdArg;
};

class tMod {
public:
	tMod(const std::string &szFileName);

	void toMod(const std::string &szFileName, bool isSkipSampleData);

	const std::vector<tSample> &getSamples(void) const;

	const std::string &getSongName(void) const;

	void setSamples(std::vector<tSample> &vSamplesNew);

	/**
	 * @brief Reorder samples in module using the index array.
	 * Reorder values for empty samples are ignored.
	 *
	 * @param vNewOrder Vector index is old sample idx, value is new sample idx.
	 */
	void reorderSamples(const std::vector<uint8_t> vNewOrder);

	void clearSampleData(void);

private:
	std::string m_szSongName;
	std::uint8_t m_ubArrangementLength; ///< End pattern idx? Jumps are possible.
	std::uint8_t m_ubSongEndPos;
	std::string m_szFileFormatTag;  ///< Should be "M.K." for 31-sample format.

	std::vector<tSample> m_vSamples;

	std::vector<std::vector<std::array<tNote, 4>>> m_vPatterns;

	/**
	 * @brief Song arrangement list (pattern sequence table).
	 * These list up to 128 pattern numbers and the order they should be played in.
	 */
	std::vector<uint8_t> m_vArrangement;
};

#endif // _ACE_TOOLS_COMMON_MOD_H_
