#ifndef GUARD_ACE_MANAGER_COPPER_H
#define GUARD_ACE_MANAGER_COPPER_H

/**
 * Double-buffered copper manager - one to rule them all.
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
 * BEWARE: if you plan using raw buffer access, you'll have to better know
 * viewport managers internals to know how they work and to interface with them
 * corretly. Some of them are only made to use copperblocks, so they may be
 * unusable without them. Some things you'll have to implement by yourself.
 * You have been warned.
 */

#ifdef AMIGA
#include <hardware/dmabits.h> // DMAF defines

#include <ace/types.h>
#include <ace/managers/log.h>
#include <ace/managers/window.h>
#include <ace/utils/custom.h>

// Since copperlist is double buffered, status flags must be propagated for 2 passes

#define TAG_COPPER_LIST_MODE (TAG_USER|1)
#define TAG_COPPER_RAW_COUNT (TAG_USER|2)

// Values for TAG_COPPER_LIST_TYPE
#define COPPER_MODE_BLOCK 0
#define COPPER_MODE_RAW   1

#define STATUS_REALLOC_PREV 1
#define STATUS_REALLOC_CURR 2
#define STATUS_REALLOC (1|2)  /// Block added/removed - realloc merged memory

#define STATUS_UPDATE_PREV 4
#define STATUS_UPDATE_CURR 8
#define STATUS_UPDATE (4|8)   /// Blocks changed content
#define STATUS_REORDER 16     /// Blocks changed order

/* ******************************************************************** TYPES */

typedef struct {
	// Higher word
	unsigned bfUnused  :7;  /// Always set to 0
	unsigned bfDestAddr:9;  /// Register offset from &custom segment
	                        /// LSBit must be set to 0 - WAIT check
	// Lower word
	unsigned bfValue:16;    /// New value
} tCopMoveCmd;

typedef struct {
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

typedef union {
	tCopMoveCmd sMove;
	tCopWaitCmd sWait;
	ULONG ulCode;
} tCopCmd;

typedef struct {
	UWORD uwAllocSize; /// Allocated memory size
	UWORD uwCmdCount;  /// Copper command count
	tCopCmd *pList;    /// HW Copperlist pointer
} tCopBfr;

typedef struct _tCopBlock {
	struct _tCopBlock *pNext;
	tUwCoordYX uWaitPos;         /// Wait pos YX
	UWORD uwMaxCmds;             /// Command limit
	UWORD uwCurrCount;           /// Curr instruction count
	UBYTE ubDisabled;            /// 1: disabled, 0: enabled
	UBYTE ubUpdated;             /// 2: curr update, 1: prev update, 0: no update
	UBYTE ubResized;             /// 2: curr size change, 1: prev size change, 0: no change
	tCopCmd *pCmds;              /// Command pointer
} tCopBlock;

typedef struct _tCopList {
	UWORD uwBlockCount;     /// Total number of blocks
	UBYTE ubStatus;         /// Status flags for processing
	UBYTE ubMode;           /// Sets block/raw mode
	tCopBfr *pFrontBfr;     /// Currently displayed copperlist
	tCopBfr *pBackBfr;      /// Editable copperlist
	tCopBlock *pFirstBlock; /// Block list
} tCopList;

typedef struct {
	tCopList *pCopList;   /// Currently displayed tCopList
	tCopList *pBlankList; /// Empty copperlist - LoadView(0) equivalent
} tCopManager;

/* ****************************************************************** GLOBALS */

extern tCopManager g_sCopManager;

/* **************************************************************** FUNCTIONS */

/********************* Copper manager functions *******************************/

void copCreate(void);
void copDestroy(void);
void copSwapBuffers(void);
void copDumpBlocks(void);
void copDumpBfr(
	IN tCopBfr *pBfr
);

/********************* Copper list functions **********************************/

tCopList *copListCreate(void *pTagList, ...);

/**
 * Destroys copperlist along with all attached blocks.
 */
void copListDestroy(
	IN tCopList *pCopList
);

/********************* Copper block functions *********************************/

/**
 * Creates new copperlist instruction block with given command count,
 * automatically WAITing for given x,y.
 * Block creation sets STATUS_REORDER on parent copperlist, so they don't need
 * to be added in order.
 */
tCopBlock *copBlockCreate(
	IN tCopList *pCopList,
	IN UWORD uwMaxCmds,
	IN UWORD uwWaitX,
	IN UWORD uwWaitY
);

void copBlockDestroy(
	IN tCopList *pCopList,
	IN tCopBlock *pBlock
);

/**
 * Disables instruction block, so it will be omitted during copperlist merge.
 */
void copBlockDisable(
	IN tCopList *pCopList,
	IN tCopBlock *pBlock
);

/**
 * Enables previously disabled instruction block.
 */
void copBlockEnable(
	IN tCopList *pCopList,
	IN tCopBlock *pBlock
);

/**
 * Reallocs current backBfr so that it will fit all copper blocks
 */
UBYTE copBfrRealloc(void);

/**
 * Reorders whole copper block list
 */
void copReorderBlocks(void);

UBYTE copUpdateFromBlocks(void);

void copProcessBlocks(void);

/**
 *  Adds copBlock which disables given sprites.
 *  Resulting copBlock is placed at 0,0 so that it will be executed during VBlank.
 *
 *  @param pList         Copperlist to be edited.
 *  @param fubSpriteMask Determines sprites to be disabled.
 *                       Setting bit0 to 1 disables sprite 0, etc.
 *
 *  @return Pointer to resulting copBlock.
 */
tCopBlock *copBlockDisableSprites(
	IN tCopList *pList,
	IN FUBYTE fubSpriteMask
);

/********************* Copperblock cmd functions ******************************/

/**
 *  Changes WAIT position for given copper block
 *  Wait may result in copper block reorder - setting STATUS_REORDER
 *  in copperlist lies on user's hands. This is not done automatically for
 *  performance sake.
 *
 *  @param pCopList Parent copperlist.
 *  @param pBlock   CopBlock to be modified.
 *  @param uwX      WAIT cmd's X position.
 *  @param uwY      Ditto, Y.
 */
void copBlockWait(
	IN tCopList *pCopList,
	INOUT tCopBlock *pBlock,
	IN UWORD uwX,
	IN UWORD uwY
);

/**
 *  Appends MOVE command to end of copper block.
 *
 *  @param pCopList Parent copperlist
 *  @param pBlock   CopBlock to be modified
 *  @param pReg     Custom chip register address to be set
 *  @param uwValue  New register's value.
 */
void copMove(
	IN tCopList *pCopList,
	INOUT tCopBlock *pBlock,
	IN volatile void *pReg,
	IN UWORD uwValue
);

/********************* Lowlevel-ish cmd functions *****************************/

/**
 *  Prepares WAIT command on given memory address.
 *  This fn is relatively slow for editing copperlist, since it builds whole WAIT
 *  cmd from scratch. If you exactly know what you're doing, you can just adjust
 *  wait pos of already generated WAIT cmd and omit applying same values to rest
 *  of fields.
 *
 *  @param pWaitCmd Pointer to copper command to be modified.
 *  @param ubX WAIT cmd's X position.
 *  @param ubY Ditto, Y.
 */
void copSetWait(
	INOUT tCopWaitCmd *pWaitCmd,
	IN UBYTE ubX,
	IN UBYTE ubY
);

/**
 *  Prepares MOVE command on given memory address.
 *  This fn is relatively slow for editing copperlist, since it builds whole MOVE
 *  cmd from scratch. If you want to change only register addr or only value,
 *  edit command using its bitfields.
 *
 *  @param pMoveCmd Pointer to copper command to be modified.
 *  @param pReg     Custom chip register address to be set
 *  @param uwValue  New register's value.
 */
void copSetMove(
	INOUT tCopMoveCmd *pMoveCmd,
	volatile void *pReg,
	UWORD uwValue
);

/**
 * Disables given sprites on supplied copperlist at given cmd offset.
 * This function doesn't add any WAIT cmd, be sure to put those cmds in VBlank.
 * Number of MOVE instructions added equals two times number of sprites disabled.
 *
 * @param pList         Copperlist to be edited.
 * @param fubSpriteMask Determines sprites to be disabled.
 *                       Setting bit0 to 1 disables sprite 0, etc.
 * @return Number of MOVE instructions added.
 */
FUBYTE copRawDisableSprites(
	IN tCopList *pList,
	IN FUBYTE fubSpriteMask,
	IN FUWORD fuwCmdOffs
);

#endif // AMIGA
#endif // GUARD_ACE_MANAGER_COPPER_H
