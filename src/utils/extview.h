#ifndef GUARD_ACE_UTIL_EXTVIEW_H
#define GUARD_ACE_UTIL_EXTVIEW_H

#include <clib/exec_protos.h>     // Amiga typedefs
#include <clib/graphics_protos.h> // BitMap etc.

#include "config.h"
#include "managers/log.h"
#include "managers/memory.h"
#include "managers/copper.h"

/* Types */

#define V_GLOBAL_CLUT 1

#define VP_NOCLUT 1

// identyfikatory mened¿erów viewporta
// numer determinuje kolejnoœæ na liœcie a zatem odœwie¿ania
// kamera na koñcu, ¿eby reszta mog³a wykryæ ruch
#define VPM_SCROLL       0
#define VPM_TILEBUFFER   1
#define VPM_DOUBLEBUFFER 2
#define VPM_CAMERA       128


/**
 * ViewPort manager structure
 * Only process and destroy included, each manager has different init params
 */
typedef struct _tVpManager {
	struct _tVpManager *pNext;
	void  (*process)(struct _tVpManager *pManager);
	void  (*destroy)(struct _tVpManager *pManager);
	struct _tVPort *pVPort;                         /// Quick ref to VPort
	UBYTE ubId;
} tVpManager;

typedef void (*tVpManagerFn)(tVpManager *pManager);

typedef struct {
	UBYTE ubVpCount;
	UWORD uwFlags;
	struct _tCopList *pCopList;
	struct _tVPort *pFirstVPort;
} tView;

typedef struct _tVPort {
	// Main
	tView *pView;              /// Pointer to parent tView
	struct _tVPort *pNext;     /// Pointer to next tVPort
	tVpManager *pFirstManager; /// tVpManager list
	UWORD uwFlags;
	// VPort dimensions
	UWORD uwOffsX;
	UWORD uwOffsY;
	UWORD uwWidth;
	UWORD uwHeight;
	// Color info
	UBYTE ubBPP;               /// Bitplane count
	UWORD *pPalette;           /// Destination palette
} tVPort;

/* Globals */

/* Functions */

/**************************** View functions **********************************/
tView *viewCreate(
	IN UWORD uwFlags
);
void viewDestroy(
	IN tView *pView
);
void viewProcessManagers(
	IN tView *pView
);

void viewUpdateCLUT(
	IN tView *pView
);

void viewLoad(
	IN tView *pView
);

/**************************** Viewport functions ******************************/
tVPort *vPortCreate(
	IN tView *pView,
	IN UWORD uwWidth, 
	IN UWORD uwHeight,
	IN UBYTE ubBPP,
	IN UWORD uwFlags
);
void vPortDestroy(
	IN tVPort *pVPort
);

/**************************** Viewport manager functions **********************/
void vPortAddManager(
	IN tVPort *pVPort,
	IN tVpManager *pVpManager
);
void vPortRmManager(
	IN tVPort *pVPort,
	IN tVpManager *pVpManager
);
tVpManager *vPortGetManager(
	IN tVPort *pVPort,
	IN UBYTE ubId
);

/**************************** Viewport copperblock functions ******************/
struct UCopList *vPortAddCopperBlock(
	IN tVPort *pVPort,
	IN UWORD uwLength
);
void vPortRmCopperBlock(
	IN tVPort *pVPort,
	IN struct UCopList *pUCopList
);

/**************************** View fade functions *****************************/
void extViewFadeOut(IN tView *pView);

void extViewFadeIn(IN tView *pView);

#endif