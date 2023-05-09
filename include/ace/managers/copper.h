/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_MANAGERS_COPPER_H_
#define _ACE_MANAGERS_COPPER_H_

/**
 * @file copper.h
 * @brief Double-buffered copper manager - one to rule them all.
 * Implements copperlist buffers and lowlevel-ish copper cmds.
 *
 * For convenience at cost of speed, one may use copper blocks - MOVE cmds
 * grouped together by single WAIT cmd. They are automatically reordered
 * if needed and may be used for small MOVE groups moving across the list.
 * This approach is superior with fast CPU and true FAST ram.
 *
 * If one needs quicker copperlist access on bare machine, one may write
 * directly into buffers inside CHIP ram using lowlevel-ish functions, command
 * bitfield modifications or even blits. Lastly, buffer swap fn must be called.
 *
 * @warning if you plan using raw buffer access, you'll have to better know
 * viewport managers internals to know how they work and to interface with them
 * corretly. Some of them are only made to use copperblocks, so they may be
 * unusable without them. Some things you'll have to implement by yourself.
 * You have been warned.
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef AMIGA
#include <hardware/dmabits.h> // DMAF defines

#include <ace/types.h>
#include <ace/managers/log.h>
#include <ace/utils/custom.h>
#include <ace/utils/tag.h>

typedef enum tCopListCreateTags {
	TAG_COPPER_LIST_MODE = (TAG_USER|1),
	TAG_COPPER_RAW_COUNT = (TAG_USER|2),
} tCopListCreateTags;

// Values for TAG_COPPER_LIST_TYPE
#define COPPER_MODE_BLOCK 0
#define COPPER_MODE_RAW   1

// Since copperlist is double buffered, status flags must be propagated for 2 passes

#define STATUS_REALLOC_PREV 1
#define STATUS_REALLOC_CURR 2
#define STATUS_REALLOC (1|2)  /// Block added/removed - realloc merged memory

#define STATUS_UPDATE_PREV 4
#define STATUS_UPDATE_CURR 8
#define STATUS_UPDATE (4|8)   /// Blocks changed content
#define STATUS_REORDER 16     /// Blocks changed order

//------------------------------------------------------------------------ TYPES

typedef struct tCopMoveCmd {
	// Higher word
	unsigned bfUnused  :7;  /// Always set to 0
	unsigned bfDestAddr:9;  /// Register offset from &custom segment
	                        /// LSBit must be set to 0 - WAIT check
	// Lower word
	unsigned bfValue:16;    /// New value
} tCopMoveCmd;

typedef struct _tCopWaitCmd {
	// Higher word
	unsigned bfWaitY        :8; /// Y position
	unsigned bfWaitX        :7; /// X position
	unsigned bfIsWait       :1; /// Always set to 1
	// Lower word
	unsigned bfBlitterIgnore:1; /// If set to 0, waits for pos and blit finish
	unsigned bfVE           :7; /// Y compare enable bits
	unsigned bfHE           :7; /// X compare enable bits
	unsigned bfIsSkip       :1; /// Set to 1 for SKIP, 0 for WAIT
} tCopWaitCmd;

typedef union _tCopCmd {
	tCopMoveCmd sMove;
	tCopWaitCmd sWait;
	ULONG ulCode;
} tCopCmd;

typedef struct _tCopBfr {
	UWORD uwAllocSize; /// Allocated memory size
	UWORD uwCmdCount;  /// Copper command count
	tCopCmd *pList;    /// HW Copperlist pointer
} tCopBfr;

typedef struct _tCopBlock {
	struct _tCopBlock *pNext;
	tUwCoordYX uWaitPos; /// Wait pos YX
	UWORD uwMaxCmds;     /// Command limit
	UWORD uwCurrCount;   /// Curr instruction count
	UBYTE ubDisabled;    /// 1: disabled, 0: enabled
	UBYTE ubUpdated;     /// 2: curr update, 1: prev update, 0: no update
	UBYTE ubResized;     /// 2: curr size change, 1: prev size change, 0: no change
	tCopCmd *pCmds;      /// Command pointer
} tCopBlock;

typedef struct _tCopList {
	UWORD uwBlockCount;     /// Total number of blocks
	UBYTE ubStatus;         /// Status flags for processing
	UBYTE ubMode;           /// Sets block/raw mode
	tCopBfr *pFrontBfr;     /// Currently displayed copperlist
	tCopBfr *pBackBfr;      /// Editable copperlist
	tCopBlock *pFirstBlock; /// Block list
} tCopList;

typedef struct _tCopManager {
	tCopList *pCopList;   /// Currently displayed tCopList
	tCopList *pBlankList; /// Empty copperlist - LoadView(0) equivalent
} tCopManager;

/**
 * @brief Values for composing bit mask of sprites.
 * Note that the first sprite is called "0" because this way it is consistent
 * with odd/even sprite naming which is widespread in literature.
 */
typedef enum tSpriteMask {
	SPRITE_0 = BV(0),
	SPRITE_1 = BV(1),
	SPRITE_2 = BV(2),
	SPRITE_3 = BV(3),
	SPRITE_4 = BV(4),
	SPRITE_5 = BV(5),
	SPRITE_6 = BV(6),
	SPRITE_7 = BV(7),
} tSpriteMask;

//---------------------------------------------------------------------- GLOBALS

extern tCopManager g_sCopManager;

//-------------------------------------------------------------------- FUNCTIONS

/********************* Copper manager functions *******************************/

void copCreate(void);
void copDestroy(void);
void copSwapBuffers(void);
void copDumpBlocks(void);
void copDumpBfr(tCopBfr *pBfr);

/********************* Copper list functions **********************************/

tCopList *copListCreate(void *pTagList, ...);

/**
 * @brief Destroys copperlist along with all attached blocks.
 */
void copListDestroy(tCopList *pCopList);

/********************* Copper block functions *********************************/

/**
 * @brief Creates new copperlist instruction block with given command count,
 * automatically WAITing for given x,y.
 *
 * Block creation sets STATUS_REORDER on parent copperlist, so they don't need
 * to be added in order.
 */
tCopBlock *copBlockCreate(
	tCopList *pCopList, UWORD uwMaxCmds, UWORD uwWaitX, UWORD uwWaitY
);

void copBlockDestroy(tCopList *pCopList,tCopBlock *pBlock);

/**
 * @brief Disables instruction block, so it will be omitted during copperlist
 * merge.
 */
void copBlockDisable(tCopList *pCopList,tCopBlock *pBlock);

/**
 * @brief Enables previously disabled instruction block.
 */
void copBlockEnable(tCopList *pCopList,tCopBlock *pBlock);

/**
 * @brief Prcesses current state of copper blocks into unified copperlist buffer.
 *
 * This function takes care of block reordering, buffer reallocating
 * and updating its contents from blocks which were marked as dirty.
 * If you need fine-grained control, you can use separate functions for those steps.
 *
 * Usually, it's best to use this function at the end of your frame loop code.
 *
 * @see copBfrRealloc()
 * @see copUpdateFromBlocks()
 * @see copReorderBlocks()
 */
void copProcessBlocks(void);

/**
 * @brief Reallocs current backBfr so that it will fit all copper blocks
 *
 * @return New status value of copper block.
 */
UBYTE copBfrRealloc(void);

/**
 * @brief Updates contents of current copperlist backbuffer with contents
 * of copper blocks.
 *
 * @return New status value of copper block.
 */
UBYTE copUpdateFromBlocks(void);

/**
 * @brief Reorders whole copper block list.
 */
void copReorderBlocks(void);

/********************* Copperblock cmd functions ******************************/

/**
 * @brief Changes WAIT position for given copper block.
 *
 * Wait may result in copper block reorder - setting STATUS_REORDER
 * in copperlist lies on user's hands. This is not done automatically for
 * performance sake.
 *
 * @param pCopList Parent copperlist.
 * @param pBlock CopBlock to be modified.
 * @param uwX WAIT cmd's X position.
 * @param uwY Ditto, Y.
 */
void copBlockWait(tCopList *pCopList, tCopBlock *pBlock, UWORD uwX, UWORD uwY);

/**
 * @brief Appends MOVE command to end of copper block.
 *
 * @param pCopList Parent copperlist
 * @param pBlock CopBlock to be modified
 * @param pReg Custom chip register address to be set
 * @param uwValue New register's value.
 */
void copMove(
	tCopList *pCopList, tCopBlock *pBlock, volatile void *pReg, UWORD uwValue
);

/********************* Lowlevel-ish cmd functions *****************************/

/**
 * @brief Prepares WAIT command on given memory address.
 *
 * @note This fn is relatively slow for editing copperlist, since it builds
 * whole WAIT cmd from scratch.
 * If you exactly know what you're doing, you can just adjust wait pos
 * of already generated WAIT cmd and omit applying same values to rest of fields.
 *
 * @param pWaitCmd Pointer to copper command to be modified.
 * @param ubX WAIT cmd's X position.
 * @param ubY Ditto, Y.
 */
void copSetWait(tCopWaitCmd *pWaitCmd, UBYTE ubX, UBYTE ubY);

/**
 * @brief Prepares MOVE command on given memory address.
 *
 * @note This fn is relatively slow for editing copperlist, since it builds
 * whole WAIT cmd from scratch.
 * If you exactly know what you're doing, you can just adjust wait pos
 * of already generated WAIT cmd and omit applying same values to rest of fields.
 *
 * @param pMoveCmd Pointer to copper command to be modified.
 * @param pReg Custom chip register address to be set
 * @param uwValue New register's value.
 *
 * @see copSetMoveVal()
 */
void copSetMove(tCopMoveCmd *pMoveCmd, volatile void *pReg, UWORD uwValue);

/**
 * @brief Sets the MOVE command target value to a new one.
 * This is way faster than calling copSetMove() repeatedly if you're just
 * changing the value.
 *
 * @param pMoveCmd Pointer to copper command to be modified.
 * @param uwValue  New register's value. The target register doesn't change.
 *
 * @see copSetMoveVal()
 */
static inline void copSetMoveVal(tCopMoveCmd *pMoveCmd, UWORD uwValue) {
	pMoveCmd->bfValue = uwValue;
}

#endif // AMIGA

#ifdef __cplusplus
}
#endif

#endif // _ACE_MANAGERS_COPPER_H_
