/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/ptplayer.h>

void ptplayerCreate(void) {

}

void ptplayerDestroy(void) {

}

void ptplayerProcess(void) {

}

void ptplayerLoadMod(
	tPtplayerMod *pMod, tPtplayerSamplePack *pSamples, UWORD uwInitialSongPos
) {

}

UBYTE ptplayerModIsCurrent(const tPtplayerMod *pMod) {
	return 0;
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

void ptplayerSfxStopOnChannel(UBYTE ubChannel) {

}

void ptplayerConfigureSongRepeat(UBYTE isRepeat, tPtplayerCbSongEnd cbSongEnd) {

}

void ptplayerWaitForSfx(void) {

}

tPtplayerSamplePack *ptplayerSamplePackCreate(const char *szPath) {
	return 0;
}

void ptplayerSamplePackDestroy(tPtplayerSamplePack *pSamplePack) {

}

//-------------------------------------------------------------------------- SFX

void muteChannelsPlayingSfx(const tPtplayerSfx *pSfx) {

}

void ptplayerSfxPlay(
	const tPtplayerSfx *pSfx, UBYTE ubChannel, UBYTE ubVolume, UBYTE ubPriority
) {

}

void ptplayerSfxPlayLooped(
	const tPtplayerSfx *pSfx, UBYTE ubChannel, UBYTE ubVolume
) {

}
