#include "mouse.h"

/* Globals */
tMouseManager g_sMouseManager;

/* Functions */
void mouseOpen() {
	g_sMouseManager.pInputMP = CreatePort(NULL, 0L);
	g_sMouseManager.pInputIO = (struct IOStdReq *) CreateExtIO(g_sMouseManager.pInputMP, sizeof(struct IOStdReq));

	OpenDevice("input.device", 0, (struct IORequest *) g_sMouseManager.pInputIO, 0);
}

void mouseSetState(UBYTE ubMouseCode, UBYTE ubMouseState) {
	ubMouseCode -= IECODE_LBUTTON;
	g_sMouseManager.pStates[ubMouseCode] = ubMouseState;
}

UBYTE mouseCheck(UBYTE ubMouseCode) {
	ubMouseCode -= IECODE_LBUTTON;
	return g_sMouseManager.pStates[ubMouseCode] != MOUSE_NACTIVE;
}

UBYTE mouseUse(UBYTE ubMouseCode) {
	ubMouseCode -= IECODE_LBUTTON;
	if (g_sMouseManager.pStates[ubMouseCode] == MOUSE_ACTIVE) {
		g_sMouseManager.pStates[ubMouseCode] = MOUSE_USED;
		return 1;
	}
	return 0;
}

UBYTE mouseIsIntersects(UWORD uwX, UWORD uwY, UWORD uwWidth, UWORD uwHeight) {
	return (uwX <= g_sWindowManager.pWindow->MouseX) && (g_sWindowManager.pWindow->MouseX < uwX + uwWidth) && (uwY <= g_sWindowManager.pWindow->MouseY) && (g_sWindowManager.pWindow->MouseY < uwY + uwHeight);
}

inline UWORD mouseGetX(void) {
	return g_sWindowManager.pWindow->MouseX;
}

inline UWORD mouseGetY(void) {
	return g_sWindowManager.pWindow->MouseY;
}

inline void mouseSetPointer(UWORD *pCursor, WORD wHeight, WORD wWidth, WORD wOffsetX, WORD wOffsetY) {
	logBlockBegin("mouseSetPointer(pCursor: %p, wHeight: %d, wWidth, %d, wOffsetX: %d, wOffsetY: %d)", pCursor, wHeight, wWidth, wOffsetX, wOffsetY);
	SetPointer(g_sWindowManager.pWindow, pCursor, wHeight, wWidth, wOffsetX, wOffsetY);
	logBlockEnd("mouseSetPointer");
}

inline void mouseResetPointer(void) {
	ClearPointer(g_sWindowManager.pWindow);
}

void _mouseDo(struct InputEvent *pEvent) {
	g_sMouseManager.pInputIO->io_Data = (APTR) pEvent;
	g_sMouseManager.pInputIO->io_Length = sizeof(struct InputEvent);
	g_sMouseManager.pInputIO->io_Command = IND_WRITEEVENT;
	
	DoIO((struct IORequest *) g_sMouseManager.pInputIO);
}

void mouseMove(WORD wX, WORD wY) {
	struct InputEvent *pEvent = memAllocChipFlags(sizeof(struct InputEvent), MEMF_PUBLIC | MEMF_CLEAR);

	pEvent->ie_Class = IECLASS_POINTERPOS;
	pEvent->ie_Code = IECODE_NOBUTTON;
	pEvent->ie_Qualifier = IEQUALIFIER_RELATIVEMOUSE; //pozycja wzglêdna
	pEvent->ie_X = wX;
	pEvent->ie_Y = wY;

	_mouseDo(pEvent);

	memFree(pEvent, sizeof(struct InputEvent));
}

void mouseClick(UBYTE ubMouseCode) {
	struct InputEvent *pEvent = memAllocChipFlags(sizeof(struct InputEvent), MEMF_PUBLIC | MEMF_CLEAR);

	pEvent->ie_Class = IECLASS_RAWMOUSE;
	pEvent->ie_Code = ubMouseCode;

	_mouseDo(pEvent);

	memFree(pEvent, sizeof(struct InputEvent));
}

void mouseClose() {
	CloseDevice((struct IORequest *) g_sMouseManager.pInputIO);
	DeleteExtIO((struct IORequest *) g_sMouseManager.pInputIO);
	DeletePort(g_sMouseManager.pInputMP);
}