#include <ace/managers/window.h>
#include <ace/generic/screen.h>

/* Globals */
#ifdef AMIGA
tWindowManager g_sWindowManager;
struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
#endif // AMIGA

/* Functions */
void windowCreate() {
	logBlockBegin("windowCreate");
#ifdef AMIGA

	if (!(IntuitionBase = (struct IntuitionBase *) OpenLibrary((CONST_STRPTR)"intuition.library", 0L))) {
		windowKill("Can't open Intuition Library!\n");
	}

	if (!(GfxBase = (struct GfxBase *) OpenLibrary((CONST_STRPTR)"graphics.library", 0L))) {
		windowKill("Can't open Gfx Library!\n");
	}

	// Screen to cover whole lores
	struct NewScreen sScreen;
	sScreen.LeftEdge = 0;
	sScreen.TopEdge = 0;
	sScreen.Width = SCREEN_PAL_WIDTH;
	sScreen.Height = SCREEN_PAL_HEIGHT;
	sScreen.Depth = 1;
	sScreen.DetailPen = 0;
	sScreen.BlockPen = 1;
	sScreen.ViewModes = 0;
	sScreen.Type = CUSTOMSCREEN;
	sScreen.Font = 0;
	sScreen.DefaultTitle = 0;
	sScreen.Gadgets = 0;
	sScreen.CustomBitMap = 0;

	if (!(g_sWindowManager.pScreen = OpenScreen(&sScreen))) {
		windowKill("Can't open Screen!\n");
	}

	ShowTitle(g_sWindowManager.pScreen, 0);

	// Window to cover whole screen - mouse hook
	struct NewWindow sWindow;
	sWindow.LeftEdge = 0;
	sWindow.TopEdge = 0;
	sWindow.Width = SCREEN_PAL_WIDTH;
	sWindow.Height = SCREEN_PAL_HEIGHT;
	sWindow.DetailPen = 0;
	sWindow.BlockPen = 1;
	sWindow.IDCMPFlags = IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY;
	sWindow.Flags = NOCAREREFRESH | BORDERLESS | RMBTRAP | ACTIVATE;
	sWindow.FirstGadget = 0;
	sWindow.CheckMark = 0;
	sWindow.Title = 0;
	sWindow.Screen = g_sWindowManager.pScreen;
	sWindow.BitMap = 0;
	sWindow.MinWidth = SCREEN_PAL_WIDTH;
	sWindow.MinHeight = SCREEN_PAL_HEIGHT;
	sWindow.MaxWidth = SCREEN_PAL_WIDTH;
	sWindow.MaxHeight = SCREEN_PAL_HEIGHT;
	sWindow.Type = CUSTOMSCREEN;

	if (!(g_sWindowManager.pWindow = OpenWindow(&sWindow)))
		windowKill("Can't open Window!\n");

	g_sWindowManager.pSysView = GfxBase->ActiView;

#endif // AMIGA
	logBlockEnd("windowCreate");
}

void windowDestroy() {
	logBlockBegin("windowDestroy()");
#ifdef AMIGA

	// logWrite("Restoring system view...");
	// custom.cop1lc = (ULONG)GfxBase->copinit;
	// custom.copjmp1 = 1;
	// LoadView(g_sWindowManager.pSysView);
	// WaitTOF();
	// WaitTOF();
	// logWrite("OK\n");

	logWrite("Closing intuition window...");
	if (g_sWindowManager.pWindow)
		CloseWindow(g_sWindowManager.pWindow);
	logWrite("OK\n");

	logWrite("Closing intuition screen...");
	if (g_sWindowManager.pScreen)
		CloseScreen(g_sWindowManager.pScreen);
	logWrite("OK\n");

	logWrite("Closing graphics.library...");
	if (GfxBase)
		CloseLibrary((struct Library *) GfxBase);
	logWrite("OK\n");

	logWrite("Closing intuition.library...");
	if (IntuitionBase)
		CloseLibrary((struct Library *) IntuitionBase);
	logWrite("OK\n");

#endif // AMIGA
	logBlockEnd("windowDestroy()");
}

void windowKill(char *szError) {
	logWrite(szError);
	printf(szError);

	windowDestroy();
	exit(EXIT_FAILURE);
}
