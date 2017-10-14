#include <ace/utils/bmframe.h>

void bmFrameDraw(tBitMap *pFrameSet, tBitMap *pDest, UWORD uwX, UWORD uwY, UBYTE ubTileWidth, UBYTE ubTileHeight) {
	UBYTE i;
	// Rogi
	blitCopyAligned(pFrameSet, 0,0, pDest, uwX, uwY, 16, 16);
	blitCopyAligned(pFrameSet, 32,0, pDest, uwX+((ubTileWidth-1)<<4), uwY, 16, 16);
	blitCopyAligned(pFrameSet, 0,32, pDest, uwX, uwY+((ubTileHeight-1)<<4), 16, 16);
	blitCopyAligned(pFrameSet, 32,32, pDest, uwX+((ubTileWidth-1)<<4), uwY+((ubTileHeight-1)<<4), 16, 16);
	// brzegi poziome
	for(i = 1; i < ubTileWidth-1; ++i) {
		blitCopyAligned(pFrameSet, 16,0, pDest, uwX+(i<<4), uwY, 16, 16);
		blitCopyAligned(pFrameSet, 16,32, pDest, uwX+(i<<4), uwY+((ubTileHeight-1)<<4), 16, 16);
	}
	// �rodek
	if(ubTileHeight > 2) {
		// pojedyncze brzegi pionowe
		blitCopyAligned(pFrameSet, 0,16, pDest, uwX, uwY+16, 16, 16);
		blitCopyAligned(pFrameSet, 32,16, pDest, uwX+((ubTileWidth-1)<<4), uwY+16, 16, 16);
		// pierwszy rz�d - centrum
		for(i = 1; i < ubTileWidth-1; ++i) {
			blitCopyAligned(pFrameSet, 16,16, pDest, uwX+(i<<4), uwY+16, 16, 16);
		}
		// kolejne rz�dy - zbiorczy blit
		for(i = 2; i < ubTileHeight-1; ++i) {
			blitCopyAligned(
				pDest, uwX, uwY+16, pDest, uwX, uwY+(i<<4),
				ubTileWidth<<4, 16
			);
		}
	}
}
