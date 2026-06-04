/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * @file
 */

#ifndef _DIAGNOSTICS_SCROLL_TILE_BUFFER_H_
#define _DIAGNOSTICS_SCROLL_TILE_BUFFER_H_

/**
 * @brief Creates the scroll tile buffer diagnostics state.
 */
void gsTestDiagScrollTileBufferCreate(void);

/**
 * @brief Main loop for the scroll tile buffer diagnostics state.
 */
void gsTestDiagScrollTileBufferLoop(void);

/**
 * @brief Destroys the scroll tile buffer diagnostics state.
 */
void gsTestDiagScrollTileBufferDestroy(void);

#endif // _DIAGNOSTICS_SCROLL_TILE_BUFFER_H_
