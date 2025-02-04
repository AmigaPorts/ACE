/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_GENERIC_SCREEN_H_
#define _ACE_GENERIC_SCREEN_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <ace/types.h>

#define SCREEN_XOFFSET 0x81

#define SCREEN_PAL_YOFFSET 0x2C
#define SCREEN_PAL_WIDTH 320
#define SCREEN_PAL_HEIGHT 256
#define SCREEN_PAL_HIRES_WIDTH 640
#define SCREEN_PAL_LACED_HEIGHT 512

#define SCREEN_NTSC_YOFFSET 0x22
#define SCREEN_NTSC_WIDTH 320
#define SCREEN_NTSC_HEIGHT 200
#define SCREEN_NTSC_HIRES_WIDTH 640
#define SCREEN_HTSC_LACED_HEIGHT 400


// For EHB screens to work properly, Copper has to wait for the line before
// the actual display starts and change the regs in horizontal blanking.
// Positions 0xD0, 0xD8, 0xE0 work, but 0xE2 doesn't - use the last valid copper
// wait pos so that the regs are ready just before the display.
//
// User stuff must go prior this wait pos, so it's nice to give explicit wiggle
// room.
//
// What follows is just how we arrived at those values:
// 0xE0 didn't work for KaiN in simplebuffer - 0xDC up to BPP 5 seems to
// work fine for everyone so far.
//
// For EHB (6 bitplanes) the wait pos is 0xD9, because that was the last that
// worked in simplebuffer for timfel.
//
// Vairn reports that AGA modes need a smaller value than EHB, and that 7bpp
// should use 0xC6, 8bpp 0xA0.
//
// TODO: some ppl might want to have the display set up as soon as possible
// (probably could be done earlier than 0xD0) and then e.g. start changing
// palette colors before display + right after bitplanes get displayed.
// Maybe add the switch for this behavior?
// TODO: Add a calculation on proper wait pos (less BPP - later).
// TODO: Test and find the most optional values.
// TODO: Figure out if this is more related to fetchmodes for AGA.
static const UWORD s_pCopperWaitXByBitplanes[9] = {0x00, 0xDC, 0xDC, 0xDC, 0xDC, 0xDC, 0xD9, 0xC6, 0xA0};

#ifdef __cplusplus
}
#endif

#endif // _ACE_GENERIC_SCREEN_H_
