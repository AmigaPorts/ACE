/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_UTILS_EXTVIEW_H_
#define _ACE_UTILS_EXTVIEW_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  View, viewport & viewport manager base functions.
 *  @todo viewport resolution, lace & EHB control. Managers should react accordingly.
 */

#include <ace/types.h>
#include <ace/utils/tag.h>
#include <ace/managers/log.h>
#include <ace/managers/memory.h>
#include <ace/managers/copper.h>

typedef enum tTagView {
	// Copperlist mode: raw/block
	TAG_VIEW_COPLIST_MODE      = TAG_USER | 1,
	// If in raw mode, specifies copperlist instruction count.
	TAG_VIEW_COPLIST_RAW_COUNT = TAG_USER | 2,
	// If set to non-zero, view will use first vport's palette as global & ignore other ones.
	TAG_VIEW_GLOBAL_PALETTE    = TAG_USER | 3,
	// The X value for display window start.
	TAG_VIEW_WINDOW_START_X    = TAG_USER | 4,
	// The Y value for display window start.
	TAG_VIEW_WINDOW_START_Y    = TAG_USER | 5,
	// The width of display window.
	TAG_VIEW_WINDOW_WIDTH      = TAG_USER | 6,
	// The height of display window. Defaults to (lastPalScanline - TAG_VIEW_WINDOW_START_Y)
	TAG_VIEW_WINDOW_HEIGHT     = TAG_USER | 7,
	// If set to non-zero, view will use first vport's bpp value for whole screen.
	TAG_VIEW_GLOBAL_BPP        = TAG_USER | 8,
	// If set to non-zero, view will use first vport's horizontal resolution (hires on/off) setting for whole screen.
	TAG_VIEW_GLOBAL_HRES       = TAG_USER | 9,
} tTagView;

// Values for TAG_VIEW_COPLIST_MODE
#define VIEW_COPLIST_MODE_BLOCK COPPER_MODE_BLOCK
#define VIEW_COPLIST_MODE_RAW   COPPER_MODE_RAW

typedef enum tTagVport {
	// Ptr to parent view
	TAG_VPORT_VIEW         = TAG_USER | 1,
	// vPort dimensions, in pixels
	TAG_VPORT_WIDTH        = TAG_USER | 2,
	// vPort height. Defaults to remaining space in view.
	TAG_VPORT_HEIGHT       = TAG_USER | 3,
	// vPort depth, best effects on OCS with 4 or less since copper is faster
	TAG_VPORT_BPP          = TAG_USER | 4,
	// Pointer to palette to initialize vPort with and its size, in color count.
	TAG_VPORT_PALETTE_PTR  = TAG_USER | 5,
	TAG_VPORT_PALETTE_SIZE = TAG_USER | 6,
	// Specify vertical offset from previous VPort
	// TODO auto CopBlocks for disabling bitplane DMA
	// When in raw mode, you have to disable DMA yourself, 'cuz making it work
	// automatically would mean passing additional 2 offsets for WAIT/MOVEs for
	// disabling/enabling DMA and then wasting all cycles between it for VPort
	// manager stuff without letting you including custom instructions in spare
	// time.
	TAG_VPORT_OFFSET_TOP   = TAG_USER | 7,
	// Set to 1 to enable hires mode, set to zero for lores
	TAG_VPORT_HIRES        = TAG_USER | 8,
} tTagVport;

/* Types */

/**
 *  View flags.
 */
typedef enum tViewFlags {
	VIEW_FLAG_GLOBAL_PALETTE = BV(0),
	VIEW_FLAG_COPLIST_RAW    = BV(1),
	VIEW_FLAG_GLOBAL_BPP     = BV(2),
	VIEW_FLAG_GLOBAL_HRES    = BV(3),
} tViewFlags;

/**
 * Viewport flags.
 */
typedef enum tVpFlag {
	VP_FLAG_HAS_OWN_PALETTE = BV(0),
	VP_FLAG_HIRES           = BV(1),
} tVpFlag;

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
typedef struct tVpManager {
	struct tVpManager *pNext;                      ///< Pointer to next manager.
	void  (*process)(struct tVpManager *pManager); ///< Process fn handle.
	void  (*destroy)(struct tVpManager *pManager); ///< Destroy fn handle.
	struct _tVPort *pVPort;                         ///< Quick ref to VPort.
	UBYTE ubId;                                     ///< Manager ID.
} tVpManager;

typedef void (*tVpManagerFn)(tVpManager *pManager);

/**
 *  @brief The view structure.
 *  View describes everything what goes to Amiga screen. Each view
 *  is composed of copperlist and viewports.
 */
typedef struct tView {
	UBYTE ubVpCount;             ///< Viewport count.
	UWORD uwFlags;               ///< Creation flags.
	UBYTE ubPosX;                ///< Directly populates the DIWSTRT value.
	UBYTE ubPosY;                ///< Directly populates the DIWSTRT value.
	UWORD uwWidth;
	UWORD uwHeight;
	UWORD uwBplCon0;             ///< Initial/global bplcon0 values.
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
	tVpFlag eFlags;             ///< Creation flags.

	// VPort dimensions
	UWORD uwOffsX;  ///< Viewport's X offset on view.
	UWORD uwOffsY;  ///< Viewport's Y offset on view.
	UWORD uwWidth;  ///< Viewport's width
	UWORD uwHeight; ///< Viewport's height

	// Color info
	UBYTE ubBpp;        ///< Bitplane count
	UWORD pPalette[32]; ///< Destination palette
} tVPort;

/* Globals */

/* Functions */

/*=========================== View functions =================================*/

/**
 *  @brief Creates blank tView.
 *
 *  @param pTags Pointer to tag list.
 *  @param ... Tag list, see TAG_VIEW_* defines.
 *  @return initialized View structure.
 *
 *  @see viewDestroy()
 *  @see vPortCreate()
 */
 tView *viewCreate(void *pTags, ...);

/**
 *  @brief Destroys given tView along with attached viewports.
 *
 *  @param pView View to be destroyed.
 *
 *  @see vCreate()
 *  @see vPortDestroy()
 */
void viewDestroy(tView *pView);

/**
 *  @brief Processes all viewport managers attached to view's viewports.
 *
 *  @param pView View to be processed.
 */
void viewProcessManagers(tView *pView);

/**
 *  Updates palette for view, if in global palette mode.
 *
 *  @param pView View to use the colors from.
 */
void viewUpdateGlobalPalette(const tView *pView);

/**
 *  Sets given view as current and displays it on screen.
 *
 *  @param pView View to be set as current.
 */
void viewLoad(tView *pView);

/*=========================== Viewport functions =============================*/

/**
 *  @brief Creates new tVPort inside given view with supplied dimensions and BPP.
 *  Line-spacing shouldn't be required if VPort has common palette with predecessor.
 *
 *  @param pView    Parent view
 *  @param pTagList Pointer to tag list.
 *  @param ...      Tag list, see TAG_VPORT_* defines
 *  @return initialized VPort structure.
 *
 *  @see vPortDestroy()
 */
 tVPort *vPortCreate(void *pTagList, ...);

/**
 *  @brief Destroys given tVPort along with attached managers.
 *
 *  @param pVPort Viewport to be destroyed.
 *
 *  @see vPortCreate()
 */
void vPortDestroy(tVPort *pVPort);

/**
 * @brief Waits for display beam to pass given Y-position on VPort.
 *
 * @param pVPort VPort which has given Y-position.
 * @param uwPosY Y-position on viewport.
 * @param isExact If set to 1, it will wait for exact position.
 * If set to 0, it will return immediately if current position is greater
 * or equal than end position.
 */
void vPortWaitForPos(const tVPort *pVPort, UWORD uwPosY, UBYTE isExact);

/**
 * @brief Waits for display beam to exactly reach end of given VPort.
 * It can be called multiple times to skip given amount of frames.
 * Even if it's called exactly right at the end pos, it will wait for whole next frame.
 *
 * @param pVPort VPort to be passed.
 *
 * @todo Make view offset dependent on DiWStrt.
 */
void vPortWaitForEnd(const tVPort *pVPort);

/**
 * @brief Waits for display beam to pass end of given VPort.
 * This will NOT work when calling multiple times to skip given amount of frames.
 * If beam position is past given point, it will return immediately.
 *
 * @param pVPort
 */
void vPortWaitUntilEnd(const tVPort *pVPort);

/**
 * @brief Processes all managers of given VPort.
 *
 * @param pVPort VPort of which managers should be processed.
 */
void vPortProcessManagers(tVPort *pVPort);

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
void vPortAddManager(tVPort *pVPort, tVpManager *pVpManager);

/**
 *  @brief Detaches specified manager from VPort and calls its destroy callback.
 *
 *  @param pVPort     Parent VPort.
 *  @param pVpManager VPort manager to be detached.
 *
 *  @see vPortAddManager()
 */
void vPortRmManager(tVPort *pVPort, tVpManager *pVpManager);

/**
 *  @brief Returns maanger with given ID attached to specified VPort.
 *
 *  @param pVPort Parent VPort.
 *  @param ubId   VPort manager ID to be found.
 *  @return if found, pointer to VPort manager, otherwise zero.
 */
tVpManager *vPortGetManager(tVPort *pVPort, UBYTE ubId);

/*=========================== Viewport copperblock functions =================*/

struct UCopList *vPortAddCopperBlock(tVPort *pVPort, UWORD uwLength);

void vPortRmCopperBlock(tVPort *pVPort, struct UCopList *pUCopList);

#ifdef __cplusplus
}
#endif

#endif // _ACE_UTILS_EXTVIEW_H_
