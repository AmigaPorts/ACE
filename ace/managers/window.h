#ifndef GUARD_ACE_MANAGER_WINODW_H
#define GUARD_ACE_MANAGER_WINODW_H

#include <stdlib.h>

#ifdef AMIGA
#include <clib/exec_protos.h> // Amiga typedefs
#include <clib/intuition_protos.h> // NewScreen etc
#include <clib/graphics_protos.h> // InitVPort etc
#include <graphics/gfxbase.h> // GfxBase etc
#endif // AMIGA

#include <ace/config.h>

#include <ace/utils/extview.h> // tExtView
#include <ace/managers/log.h>
#include <ace/managers/memory.h>

/* Types */
#ifdef AMIGA
typedef struct {
	struct Screen *pScreen;
	struct Window *pWindow;
	struct View *pSysView;  /// System view before running app and swapping views
} tWindowManager;

/* Globals */
extern tWindowManager g_sWindowManager;
extern struct IntuitionBase *IntuitionBase;
extern struct GfxBase *GfxBase;
#endif

/* Functions */
void windowCreate(void);

void windowDestroy(void);

void windowKill(
	IN char *szError
);

#endif
