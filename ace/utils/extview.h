#ifndef GUARD_ACE_UTIL_EXTVIEW_H
#define GUARD_ACE_UTIL_EXTVIEW_H

/**
 *  View, viewport & viewport manager base functions.
 *  @todo viewport resolution, lace & EHB control. Or in managers?
 */

#include <clib/exec_protos.h>     // Amiga typedefs
#include <clib/graphics_protos.h> // BitMap etc.
#include <ace/config.h>
#include <ace/managers/log.h>
#include <ace/managers/memory.h>
#include <ace/managers/copper.h>

/* Types */

/**
 *  View creation flags.
 */
#define V_GLOBAL_CLUT 1

/**
 *  VPort creation flags.
 */
#define VP_NOCLUT 1

/**
 *  Viewport manager IDs.
 *  Number determines processing order. Camera is last, so rest may see
 *  a difference between its current and previous position
 */
#define VPM_SCROLL       0
#define VPM_TILEBUFFER   1
#define VPM_DOUBLEBUFFER 2
#define VPM_CAMERA       128

/**
 *  @brief ViewPort manager structure.
 *  Only process and destroy included, each manager has different init params.
 */
typedef struct _tVpManager {
	struct _tVpManager *pNext;                      ///< Pointer to next manager.
	void  (*process)(struct _tVpManager *pManager); ///< Process fn handle.
	void  (*destroy)(struct _tVpManager *pManager); ///< Destroy fn handle.
	struct _tVPort *pVPort;                         ///< Quick ref to VPort.
	UBYTE ubId;                                     ///< Manager ID.
} tVpManager;

typedef void (*tVpManagerFn)(tVpManager *pManager);

/**
 *  @brief The view structure.
 *  View describes everything what goes to Amiga screen. Each view
 *  is composed of copperlist and viewports.
 */
typedef struct _tView {
	UBYTE ubVpCount;             ///< Viewport count.
	UWORD uwFlags;               ///< Creation flags.
	struct _tCopList *pCopList;  ///< Pointer to copperlist.
	struct _tVPort *pFirstVPort; ///< Pointer to first VPort on list.
} tView;

/**
 *  @brief The viewport structure.
 *  Each viewport has specified resolution, screen dimensions and its own
 *  manager list.
 */
typedef struct _tVPort {
	// Main
	tView *pView;              ///< Pointer to parent tView.
	struct _tVPort *pNext;     ///< Pointer to next tVPort.
	tVpManager *pFirstManager; ///< Pointer to first viewport manager on list.
	UWORD uwFlags;             ///< Creation flags.
	
	// VPort dimensions
	UWORD uwOffsX;  ///< Viewport's X offset on view.
	UWORD uwOffsY;  ///< Viewport's Y offset on view.
	UWORD uwWidth;  ///< Viewport's width
	UWORD uwHeight; ///< Viewport's height
	
	// Color info
	UBYTE ubBPP;        ///< Bitplane count
	UWORD pPalette[32]; ///< Destination palette
} tVPort;

/* Globals */

/* Functions */

/*=========================== View functions =================================*/

/**
 *  @brief Creates blank tView.
 *  
 *  @param uwFlags Creation flags (V_*).
 *  @return initialized View structure.
 *  
 *  @see viewDestroy()
 *  @see vPortCreate()
 */
tView *viewCreate(
	IN UWORD uwFlags
);

/**
 *  @brief Destroys given tView along with attached viewports.
 *  
 *  @param pView View to be destroyed.
 *  
 *  @see vCreate()
 *  @see vPortDestroy()
 */
void viewDestroy(
	IN tView *pView
);

/**
 *  @brief Processes all viewport managers attached to view's viewports.
 *  
 *  @param pView View to be processed.
 */
void viewProcessManagers(
	IN tView *pView
);

/**
 *  Updates CLUT for every viewport attached to view.
 *  
 *  @param pView View to be updated.
 */
void viewUpdateCLUT(
	IN tView *pView
);

/**
 *  Sets given view as current and displays it on screen.
 *  
 *  @param pView View to be set as current.
 */
void viewLoad(
	IN tView *pView
);

/*=========================== Viewport functions =============================*/

/**
 *  @brief Creates new tVPort inside given view with supplied dimensions and BPP.
 *  Line-spacing shouldn't be required if VPorts have common CLUT.
 *  
 *  @param pView    Parent view
 *  @param uwWidth  Viewport's width.
 *  @param uwHeight Viewport's height.
 *  @param ubBpp    Viewport's depth.
 *  @param uwFlags  Viewport creation flags (VP_*).
 *  @return initialized VPort structure.
 *  
 *  @see vPortDestroy()
 */
tVPort *vPortCreate(
	IN tView *pView,
	IN UWORD uwWidth, 
	IN UWORD uwHeight,
	IN UBYTE ubBpp,
	IN UWORD uwFlags
);

/**
 *  @brief Destroys given tVPort along with attached managers.
 *  
 *  @param pVPort Viewport to be destroyed.
 *  
 *  @see vPortCreate()
 */
void vPortDestroy(
	IN tVPort *pVPort
);

/**
 *  @brief Waits for display beam to pass given VPort.
 *  
 *  @param pVPort VPort to be passed.
 *  
 *  @todo Make view offset dependent on DiWStrt.
 */
void vPortWaitForEnd(
	IN tVPort *pVPort
);

/*=========================== Viewport manager functions =====================*/

/**
 *  @brief Attaches specified VPort manager to given VPort.
 *  
 *  @param pVPort     Parent VPort.
 *  @param pVpManager VPort manager to be attached.
 *  
 *  @see vPortRmManager()
 *  @see vPortGetManager()
 */
void vPortAddManager(
	IN tVPort *pVPort,
	IN tVpManager *pVpManager
);

/**
 *  @brief Detaches specified manager from VPort and calls its destroy callback.
 *  
 *  @param pVPort     Parent VPort.
 *  @param pVpManager VPort manager to be detached.
 *  
 *  @see vPortAddManager()
 */
void vPortRmManager(
	IN tVPort *pVPort,
	IN tVpManager *pVpManager
);

/**
 *  @brief Returns maanger with given ID attached to specified VPort.
 *  
 *  @param pVPort Parent VPort.
 *  @param ubId   VPort manager ID to be found.
 *  @return if found, pointer to VPort manager, otherwise zero.
 */
tVpManager *vPortGetManager(
	IN tVPort *pVPort,
	IN UBYTE ubId
);

/*=========================== Viewport copperblock functions =================*/

struct UCopList *vPortAddCopperBlock(
	IN tVPort *pVPort,
	IN UWORD uwLength
);

void vPortRmCopperBlock(
	IN tVPort *pVPort,
	IN struct UCopList *pUCopList
);

/*=========================== View fade functions ============================*/

void extViewFadeOut(IN tView *pView);

void extViewFadeIn(IN tView *pView);

#endif