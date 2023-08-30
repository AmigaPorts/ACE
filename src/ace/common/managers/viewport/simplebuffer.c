/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/viewport/simplebuffer_private.h>

UBYTE simpleBufferIsRectVisible(
	tSimpleBufferManager *pManager,
	UWORD uwX, UWORD uwY, UWORD uwWidth, UWORD uwHeight
) {
	return (
		uwX >= pManager->pCamera->uPos.uwX - uwWidth &&
		uwX <= pManager->pCamera->uPos.uwX + pManager->sCommon.pVPort->uwWidth &&
		uwY >= pManager->pCamera->uPos.uwY - uwHeight &&
		uwY <= pManager->pCamera->uPos.uwY + pManager->sCommon.pVPort->uwHeight
	);
}
