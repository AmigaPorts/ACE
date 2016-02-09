#ifndef GUARD_ACE_MANAGER_COPPER_H
#define GUARD_ACE_MANAGER_COPPER_H

#include <hardware/dmabits.h> // DMAF defines

#include "types.h"
#include "managers/log.h"
#include "managers/window.h"
#include "utils/custom.h"

// Since copperlist is double buffered, status flags must be propagated for 2 passes
#define STATUS_REALLOC_PREV 1
#define STATUS_REALLOC_CURR 2
#define STATUS_REALLOC (1|2)  /// Block added/removed - realloc merged memory

#define STATUS_UPDATE_PREV 4
#define STATUS_UPDATE_CURR 8
#define STATUS_UPDATE (4|8)   /// Blocks changed content

#define STATUS_REORDER 16     /// Blocks changed order

/* Types */

typedef struct {
	// Higher word
	unsigned bfUnused  :7;  /// Always set to 0
	unsigned bfDestAddr:9;  /// Register offset from &custom segment
	                        /// LSBit must be set to 0 - WAIT check
	// Lower word
	unsigned bfValue   :16; /// New value
} tCopMoveCmd;

typedef struct {
	// Higher word
	unsigned bfWaitY          :8; /// Y position
	unsigned bfWaitX          :7; /// X position
	unsigned bfIsWait         :1; /// Always set to 1
	// Lower word
	unsigned bfBlitterIgnore  :1; /// If set to 0, waits for pos and blit finish
	unsigned bfVE             :7; /// Y compare enable bits
	unsigned bfHE             :7; /// X compare enable bits
	unsigned bfIsSkip         :1; /// Set to 1 for SKIP, 0 for WAIT
} tCopWaitCmd;

typedef union {
	tCopMoveCmd sMove;
	tCopWaitCmd sWait;
	ULONG ulCode;
} tCopCmd;

typedef struct {
	UWORD uwAllocSize; /// Allocated memory size
	UWORD uwCmdCount;  /// Copper command count
	tCopCmd *pList; /// HW Copperlist pointer
} tCopBfr;

typedef struct _tCopBlock {
	struct _tCopBlock *pNext;
	tUwCoordYX uWaitPos;         /// Wait pos YX
	UWORD uwMaxCmds;             /// Command limit
	UWORD uwCurrCount;           /// Curr instruction count
	UWORD uwPrevCount;           /// Prev instruction count
	UBYTE ubDisabled;            /// 1: disabled, 0: enabled
	UBYTE ubUpdated;             /// 2: current update, 1: prev update, 0: no update
	UBYTE ubResized;             /// 2: current size change, 1: prev size change, 0: no change
	tCopCmd *pCmds;           /// Command pointer
} tCopBlock;

typedef struct _tCopList {
	UWORD uwBlockCount;     /// Total number of blocks
	UBYTE ubStatus;         /// Status flags for processing
	tCopBfr *pFrontBfr;     /// Currently displayed copperlist
	tCopBfr *pBackBfr;      /// Editable copperlist
	tCopBlock *pFirstBlock; /// Block list	
} tCopList;

typedef struct {
	tCopList *pCopList;   /// Currently displayed tCopList
	tCopList *pBlankList; /// Empty copperlist - LoadView(0) equivalent
} tCopManager;

/* Globals */

extern tCopManager g_sCopManager;

/* Functions */

/********************* Copper manager functions *******************************/

void copCreate(void);
void copDestroy(void);
void copProcess(void);
void copDump(void);

/********************* Copper list functions **********************************/

tCopList *copListCreate(void);

void copListDestroy(
	IN tCopList *pCopList
);

/********************* Copper block functions *********************************/

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

void copBlockDisable(
	IN tCopList *pCopList,
	IN tCopBlock *pBlock
);

void copBlockEnable(
	IN tCopList *pCopList,
	IN tCopBlock *pBlock
);

/********************* Copper cmd functions ***********************************/

void copWait(
	IN tCopList *pCopList,
	IN tCopBlock *pBlock,
	IN UWORD uwX, 
	IN UWORD uwY
);

void copMove(
	IN tCopList *pCopList,
	IN tCopBlock *pBlock,
	IN void *reg,
	IN UWORD uwValue
);

void copSetWait(
	OUT tCopWaitCmd *pWaitCmd,
	UBYTE ubX,
	UBYTE ubY
);

#endif