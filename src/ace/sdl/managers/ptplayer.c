/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/ptplayer.h>

void ptplayerCreate(UBYTE isPal) {

}

void ptplayerDestroy(void) {

}

void ptplayerProcess(void) {

}

void ptplayerSetPal(UBYTE isPal) {

}

tPtplayerMod *ptplayerModCreate(const char *szPath) {
	return 0;
}

void ptplayerModDestroy(tPtplayerMod *pMod) {

}

void ptplayerLoadMod(
	tPtplayerMod *pMod, tPtplayerSamplePack *pSamples, UWORD uwInitialSongPos
) {

}

void ptplayerStop(void) {

}

void ptplayerSetMusicChannelMask(UBYTE ubChannelMask) {

}

void ptplayerSetMasterVolume(UBYTE ubMasterVolume) {

}

void ptplayerSetChannelsForPlayer(UBYTE ubChannelMask) {

}

void ptplayerEnableMusic(UBYTE isEnabled) {

}

UBYTE ptplayerGetE8(void) {
	return 0;
}

void ptplayerReserveChannelsForMusic(UBYTE ubChannelCount) {

}

void ptplayerSetSampleVolume(UBYTE ubSampleIndex, UBYTE ubVolume) {

}

tPtplayerSfx *ptplayerSfxCreateFromFile(const char *szPath, UBYTE isFast) {
	return 0;
}

void ptplayerSfxDestroy(tPtplayerSfx *pSfx) {

}

void ptplayerSfxPlay(
	const tPtplayerSfx *pSfx, UBYTE ubChannel, UBYTE ubVolume, UBYTE ubPriority
) {

}

void ptplayerSfxPlayLooped(
	const tPtplayerSfx *pSfx, UBYTE ubChannel, UBYTE ubVolume
) {

}

void ptplayerSfxStopOnChannel(UBYTE ubChannel) {

}

void ptplayerConfigureSongRepeat(UBYTE isRepeat, tPtplayerCbSongEnd cbSongEnd) {

}

void ptplayerWaitForSfx(void) {

}

UBYTE ptplayerSfxLengthInFrames(const tPtplayerSfx *pSfx) {
	return 0;
}

tPtplayerSamplePack *ptplayerSamplePackCreate(const char *szPath) {
	return 0;
}

void ptplayerSamplePackDestroy(tPtplayerSamplePack *pSamplePack) {

}
