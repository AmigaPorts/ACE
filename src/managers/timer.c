#include "timer.h"
#include "managers/log.h"

/* Globals */
tTimerManager g_sTimerManager;

/* Functions */

/**
 * Timer VBlank server
 * Increments frame counter
 */
__amigainterrupt __saveds void timerVBlankServer(__reg("a1") UWORD *pCounter) {
	++(*pCounter);
}

/**
 * Creates Vertical Blank server for counting frames
 */
void timerCreate(void) {
	g_sTimerManager.uwFrameCounter = 0;
	g_sTimerManager.pInt = memAllocChipClear(sizeof(struct Interrupt)); // CHIP is PUBLIC.
	
	g_sTimerManager.pInt->is_Node.ln_Type = NT_INTERRUPT;
	g_sTimerManager.pInt->is_Node.ln_Pri = -60;
	g_sTimerManager.pInt->is_Node.ln_Name = "ACE_Timer_VBL";
	g_sTimerManager.pInt->is_Data = (APTR)&g_sTimerManager.uwFrameCounter;
	g_sTimerManager.pInt->is_Code = timerVBlankServer;

	AddIntServer(INTB_VERTB, g_sTimerManager.pInt);
}

/**
 * Removes Vertical Blank server
 */
void timerDestroy(void) {
	RemIntServer(INTB_VERTB, g_sTimerManager.pInt);
	memFree(g_sTimerManager.pInt, sizeof(struct Interrupt));
}

/**
 * Gets current time based on frame number
 * One tick equals: PAL - 20ms, NTSC - 16.67ms
 * Max time capacity: 33 months
 */
ULONG timerGet(void) {
	return g_sTimerManager.uwFrameCounter;
}

/**
 * Gets as precise current time as possible
 * Implementation based on ray position and frame number
 * One tick equals: PAL - 0.40us, NTSC - 0.45us
 * Max time capacity: 1715s (28,5 min)
 */
ULONG timerGetPrec(void) {
	UWORD uwFr1, uwFr2; // frame counts
	tRayPos sRay1, sRay2;
	
	uwFr1 = g_sTimerManager.uwFrameCounter;
	CopyMem(vhPosRegs, &sRay1, 4);
	uwFr2 = g_sTimerManager.uwFrameCounter;
	CopyMem(vhPosRegs, &sRay2, 4);
	if(uwFr1 == uwFr2)
		return (uwFr1*160*313 + sRay1.uwPosY*160 + sRay1.ubPosX);
	else
		return (uwFr2*160*313 + sRay2.uwPosY*160 + sRay2.ubPosX);
}

/**
 * Gets time difference between two times
 * For use on both precise and frame time
 */
ULONG timerGetDelta(ULONG ulStart, ULONG ulStop) {
	if(ulStop >= ulStart)
		return ulStop-ulStart;
	return (0xFFFFFFFF - ulStart) + ulStop;
}

/**
 * Returns if timer has passed without updating its state
 */
BYTE timerPeek(ULONG *pTimer, ULONG ulTimerDelay) {
	return (*pTimer + ulTimerDelay) <= g_sTimerManager.ulGameTicks;
}

/**
 * Returns if timer has passed
 * If passed, its state gets resetted and countdown starts again
 */
BYTE timerCheck(ULONG *pTimer, ULONG ulTimerDelay) {
	if (timerPeek(pTimer, ulTimerDelay)) {
		*pTimer = g_sTimerManager.ulGameTicks;
		return 1;
	}
	return 0;
}

/**
 * Updates game ticks if game not paused
 */
void timerProcess(void) {
	ULONG ulCurrentTime;

	ulCurrentTime = timerGet();
	if(!g_sTimerManager.ubPaused) {
		if(ulCurrentTime > g_sTimerManager.ulLastTime)
			g_sTimerManager.ulGameTicks += ulCurrentTime - g_sTimerManager.ulLastTime;
		else
			g_sTimerManager.ulGameTicks += (0xFFFF - g_sTimerManager.ulLastTime) + ulCurrentTime + 1;
	}
	g_sTimerManager.ulLastTime = ulCurrentTime;
}

/**
 * Formats precise time to human readable form on supplied buffer
 * Current version works correctly only on ulPrecTime < 0xFFFFFFFF/4 (7 min)
 * and there seems to be no easy fix for this
 */
void timerFormatPrec(char *szBfr, ULONG ulPrecTime) {
	ULONG ulResult, ulRest;
	if(ulPrecTime > 0xFFFFFFFF>>2) {
		sprintf(szBfr, ">7min");
		return;
	}
	// ulResult [us]
	ulResult = ulPrecTime*4;
	ulRest = ulResult % 10;
	ulResult = ulResult / 10;
	if(ulResult < 1000) {
		sprintf(szBfr, "%3lu.%01lu us", ulResult, ulRest);
		return;
	}
	// ulResult [ms]
	ulRest = ulResult % 1000;
	ulResult /= 1000;
	if(ulResult < 1000) {
		sprintf(szBfr, "%3lu.%03lu ms", ulResult, ulRest);
		return;
	}
	// ulResult [s]
	ulRest = ulResult % 1000;
	ulResult /= 1000;
	sprintf(szBfr, "%lu.%03lu s", ulResult, ulRest);
}