/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * @file
 */

#ifndef _DIAGNOSTICS_SIMPLE_BUFFER_BPP_H_
#define _DIAGNOSTICS_SIMPLE_BUFFER_BPP_H_

/**
 * @brief Creates the simple buffer diagnostics state.
 */
void gsTestDiagSimpleBufferCreate(void);

/**
 * @brief Main loop for the simple buffer diagnostics state.
 */
void gsTestDiagSimpleBufferLoop(void);

/**
 * @brief Destroys the simple buffer diagnostics state.
 */
void gsTestDiagSimpleBufferDestroy(void);

#endif // _DIAGNOSTICS_SIMPLE_BUFFER_BPP_H_
