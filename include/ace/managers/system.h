#ifndef GUARD_ACE_MANAGERS_SYSTEM_H
#define GUARD_ACE_MANAGERS_SYSTEM_H

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

void systemSetInt(
	IN UBYTE ubIntNumber,
	IN tAceIntHandler pHandler,
	INOUT volatile void *pIntData
);

void systemUse(void);

void systemUnuse(void);

//---------------------------------------------------------------------- GLOBALS

#endif // GUARD_ACE_MANAGERS_SYSTEM_H
