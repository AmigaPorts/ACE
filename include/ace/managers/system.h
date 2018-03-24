#ifndef GUARD_ACE_MANAGERS_SYSTEM_H
#define GUARD_ACE_MANAGERS_SYSTEM_H

#include <graphics/gfxbase.h> // Required for GfxBase
#include <ace/types.h>
#include <ace/utils/custom.h>

//---------------------------------------------------------------------- DEFINES

//------------------------------------------------------------------------ TYPES

typedef void (*tAceIntHandler)(
	REGARG(volatile tCustom *pCustom, "a0"), REGARG(volatile void *tData, "a1")
);

//-------------------------------------------------------------------- FUNCTIONS

void systemCreate(void);

void systemDestroy(void);

void systemKill(
	IN const char *szMsg
);

void systemSetInt(
	IN UBYTE ubIntNumber,
	IN tAceIntHandler pHandler,
	INOUT volatile void *pIntData
);

void systemUse(void);

void systemUnuse(void);

void systemDump(void);

//---------------------------------------------------------------------- GLOBALS

extern struct GfxBase *GfxBase;

#endif // GUARD_ACE_MANAGERS_SYSTEM_H
